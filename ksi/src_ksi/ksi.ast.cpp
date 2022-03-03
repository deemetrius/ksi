module;

export module ksi.ast;

export namespace ksi::ast {

// precedence
enum n_prec {
	prec_leaf,
	//
	prec_top_before,
	prec_sign_before		//	-a +a
	= prec_top_before,
	//
	prec_top_after,
	prec_type_of_after		//	a$ a& a&&		(type of, get ref, get weak ref)
	= prec_top_after,
	prec_then_after			//	? then-end each
	= prec_top_after,
	prec_not_after			//	`not			(logical, unary postfix)
	= prec_top_after,
	//
	prec_member,			//	a.key a[key]
	prec_sign_after,		//	-a +a
	prec_type_of_before,	//	a$ a& a&&		(type of, get ref, get weak ref)
	prec_rt_assign_after,	//	=>
	prec_assign_before,		//	=
	prec_invoke,			//	fn! & &&		(fn call by ref)
	prec_pow,				//	**
	prec_mult,				//	* / `mod
	prec_and_bits,			//	&				(bitwise)
	prec_xor_bits,			//	^
	prec_or_bits,			//	|
	prec_plus,				//	+ - % %% #std_fn &user_fn
	prec_cmp,				//	<=>
	prec_eq,				//	== <> < <= > >=
	prec_throw,				//	`throw
	prec_nullc,				//	?? ?!			(null coalescing)
	prec_and_bool,			//	&&				(logical)
	prec_xor_bool,			//	^^
	prec_or_bool,			//	||
	prec_assign_after,		//	=
	prec_rt_assign_before,	//	=>
	prec_then_before,		//	? then-end each
	prec_and,				//	`and			(sequential)
	prec_xor,				//	`xor
	prec_or,				//	`or
	prec_not_before,		//	`not			(logical, unary postfix)
	prec_pair,				//	:				(key-value pair)
	//
	prec_root
};

} // ns ksi::ast