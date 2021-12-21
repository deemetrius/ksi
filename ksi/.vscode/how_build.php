<?php

class how_build {
	const max_depth = 16, def_cfg = [
		'platform:target' => 'win64:release'
	];
	static public $magic = ['workspace_dir' => ''], $rules = null, $is_win = null, $perm = 0777;

	static public function read_json($path) {
		if( !is_readable($path) || ($data = file_get_contents($path)) === false ) {
			throw new Exception('error: Unable to open "'. $path .'"');
		}
		if( !is_array($data = json_decode($data, true, self::max_depth)) ) {
			throw new Exception('error: json file was not parsed "'. $path .'"');
		}
		return $data;
	}

	static public function read_json_opt($path, $default) {
		try {
			return self::read_json($path);
		} catch( Exception $e ) {
			echo preg_replace('~^error\:~u', 'notice:', $e->getMessage()), "\n";
			return $default;
		}
	}

	static public function config_path($path) {
		$path = pathinfo($path);
		if( !is_array($stat = stat($dir_config = $path['dirname'])) ) {
			throw new Exception('error: Unable to get permissions on folder "'. $dir_config .'"');
		}
		self::$perm = $stat['mode'] & 0777;
		self::$magic['workspace_dir'] = preg_replace('~[/\\\\][^/\\\\]+$~u', '', $dir_config);
		//return $path['dirname'] .'/'. $path['filename'] .'.config.json';
	}

	static public function read_config($path) {
		//$path = self::config_path($path);
		self::config_path($path);
		$data = self::read_json_opt($path, self::def_cfg);
		$ret = [];
		foreach( self::def_cfg as $key => $value ) {
			if( !isset($data[$key]) || !is_string($data[$key]) ) {
				echo
					'warning: config key "', $key, '" should have string value; So we use value "',
					$value, '" instead; Check file "', $path, '"', "\n"
				;
				$ret[$key] = $value;
			} else {
				$ret[$key] = $data[$key];
			}
		}
		return $ret;
	}

	static public function deep($data, $map, $path) {
		foreach( $map as $key => $value ) {
			if( !is_array($data) || !isset($data[$value]) ) {
				throw new Exception('error: '. $key .' "'. $value .'" was not configured in file "'. $path .'"');
			}
			$data = $data[$value];
		}
		return $data;
	}

	static public function fix_path($path) {
		return str_replace('\\', '/', $path);
	}

	static public function relative_path($path, $base) {
		$path = self::fix_path($path);
		$base = self::fix_path($base);
		$pattern = '~^'. preg_quote($base, '~') .'/~u';
		return preg_replace($pattern, '', $path);
	}

	static public function magic_rules($magic) {
		$rules = [];
		foreach( $magic as $key => $value ) {
			$rules['${'. $key .'}'] = $value;
		}
		return $rules;
	}

	static public function each_rules($each) {
		$parts = pathinfo($each);
		$each_rel = self::relative_path($each, self::$magic['workspace_dir']);
		$parts_rel = pathinfo($each_rel);
		return [
			'${each.full}'					=> $each,
			'${each.full.no_extension}'		=> ($parts['dirname'] .'/'. $parts['filename']),
			'${each.full.dirname}'			=> $parts['dirname'],
			//
			'${each.rel}'					=> $each_rel,
			'${each.rel.no_extension}'		=> ($parts_rel['dirname'] .'/'. $parts['filename']),
			'${each.rel.dirname}'			=> (($dir = $parts_rel['dirname']) == '' ? '.' : $dir),
			//
			'${each.filename}'				=> $parts['basename'],
			'${each.filename.no_extension}'	=> $parts['filename'],
			'${each.extension}'				=> $parts['extension'],
		];
	}

	static public function apply_rules($rules, $text) {
		$ret = str_replace($keys = array_keys($rules), $rules, $text);
		$count = null;
		$ret = preg_replace('~\$\{[^\}]*\}~u', '', $ret, -1, $count);
		if( $count ) {
			echo 'warning: Found wrong substitutions: ', $count, "; Supported:\n", implode("\n", $keys), "\n";
		}
		return $ret;
	}

	static public function run_task_list($data, $path) {
		if( !is_array($data) || !count($data) ) {
			throw new Exception('error: Tasks should be non-empty assoc array in file "'. $path .'"');
		}
		self::$rules = self::magic_rules(self::$magic);
		foreach( $data as $key => $task ) {
			self::run_task($task, $key, $path);
		}
	}

	static public function run_task($task, $task_key, $path) {
		if( !is_array($task) ) {
			throw new Exception('warning: Task "'. $task_key .'" should be assoc array in file "'. $path .'"');
		}
		if( !isset($task[$k='command']) || !is_string($cmd = $task[$k]) ) {
			throw new Exception('warning: Task "'. $task_key .'" should contain "command" key with string value in file "'. $path .'"');
		}
		if( !isset($task[$k='args']) || !is_array($args = $task[$k]) ) {
			throw new Exception('warning: Task "'. $task_key .'" should contain "args" key with array value in file "'. $path .'"');
		}
		echo 'Perform "', $task_key, '" task:', "\n";
		$cmd = self::apply_rules(self::$rules, $cmd);
		if( isset($task[$k='each_plain']) && is_array($each_list = $task[$k]) ) {
			foreach( $each_list as $each ) {
				$rules = self::$rules + ['${each}' => $each];
				$result = self::impl_run_task($task, $task_key, $cmd, $args, $rules);
			}
		} else if( isset($task[$k='each']) && is_array($each_list = $task[$k]) ) {
			foreach( $each_list as $each ) {
				$each = self::apply_rules(self::$rules, $each);
				$rules = self::$rules + self::each_rules($each);
				$result = self::impl_run_task($task, $task_key, $cmd, $args, $rules);
			}
		} else {
			$result = self::impl_run_task($task, $task_key, $cmd, $args, self::$rules);
		}
	}

	static public function impl_run_task($task, $task_key, $cmd, $args, $rules) {
		//
		if( isset($task[$k='maybe_create_dir']) && is_string($maybe_dir = $task[$k]) ) {
			$maybe_dir = self::apply_rules($rules, $maybe_dir);
			if( !is_dir($maybe_dir) ) {
				if( is_file($maybe_dir) ) {
					throw new Exception(
						'error: Requested option "'. $k .'" in task "'. $task_key .
						'" but it is regular file "'. $maybe_dir .'"'
					);
				}
				if( !mkdir($maybe_dir, self::$perm, true) ) {
					throw new Exception(
						'error: Requested option "'. $k .'" in task "'. $task_key .
						'" but unable to create "'. $maybe_dir .'"'
					);
				}
			}
		}
		//
		$args = self::apply_rules($rules, $args);
		$result = self::run_command($cmd, $args);
		if( $result ) {
			echo 'notice: Result = ', $result, "\n";
			exit($result);
		}
		return $result;
	}

	static public function run_command($cmd, $args) {
		echo $cmd, ' ', implode(' ', $args), "\n";
		$cmd = escapeshellcmd($cmd);
		if( self::$is_win )  $cmd = '"'. $cmd .'"';
		$args = implode(' ', array_map('escapeshellarg', $args));
		$cmd .= ' '. $args;
		$result = null;
		passthru($cmd, $result);
		return $result;
	}
}

how_build::$is_win = (preg_match('~^win~ui', PHP_OS) == 1);

try {
	if( $argc < 5 || $argv[1] !== '-json' || $argv[3] !== '-config' ) {
		throw new Exception('usage: how_build.php -json <path> -config <path>');
	}
	$config = how_build::read_config($argv[4]);
	$data = how_build::read_json($json_path = $argv[2]);
	$data = how_build::deep($data, $config, $json_path);
	how_build::run_task_list($data, $json_path);
} catch( Exception $e ) {
	echo $e->getMessage(), "\n";
	exit(1);
}