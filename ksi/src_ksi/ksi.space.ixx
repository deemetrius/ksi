module;

#include "../src/pre.h"

export module ksi.space;

export import <set>;
export import just.ref;
export import just.hive;
export import ksi.function;

export namespace ksi {
	
	//namespace fs = std::filesystem;

	/*struct with_path {
		using t_path_const_pointer = const fs::path *;
		
		// data
		t_path_const_pointer	m_path;
	};
	
	template <typename T>
	struct with_hive {
		using type = T;
		using t_ref = just::ref<type,
			just::ref_traits_count<false, just::closers::compound_count<false>::template t_closer >
		>;
		using t_hive = just::hive<fs::path, t_ref>;
		using t_hive_find_result = t_hive::t_find_result;
		
		// data
		t_hive	m_hive;
		
		t_hive_find_result add_to_hive(const fs::path & p_path) {
			t_ref v_ref = new type;
			t_hive_find_result ret = m_hive.maybe_emplace(p_path, v_ref);
			if( ret.m_added ) {
				(*ret.m_value)->m_path = ret.m_key;
			}
			return ret;
		}
		
		t_hive_find_result add_to_hive(const t_ref & p_ref) {
			t_hive_find_result ret = m_hive.maybe_emplace(*p_ref->m_path, p_ref);
			if( ret.m_added ) {
				(*ret.m_value)->m_path = ret.m_key;
			}
			return ret;
		}
	};
	
	struct t_module :
		public with_path,
		public just::bases::with_ref_count
	{};
	
	struct folder :
		public with_path,
		public with_hive<t_module>,
		public just::bases::with_ref_count
	{
		// data
		fs::path	m_default_extension;
		
		folder() : m_default_extension{".define"} {}
	};
	
	struct space :
		public with_hive<folder>
	{
		using t_files = std::set<fs::path>;

		// data
		t_files		m_files;
	};*/

	struct space {
		using t_files = std::set<fs::path>;

		// data
		t_files		m_files;
	};

} // ns