module;

#include "../src/pre.h"

export module just.optr;

/*
Smart pointer 'optr' служит для учёта циклических ссылок.
Если в методе optr::clear() брошено исключение, то деструктор кладёт 'target' в "bad_targets" список.
В clear() используются: std::list и std::map
*/

export import <type_traits>;
export import <concepts>;
export import <list>;
export import <exception>;

import <memory>;
import <utility>;
import <set>;
import <ranges>;

export import <vector>;
export import <map>;

export import just.list;

export namespace just {

	struct separator {};

	template <typename T>
	concept c_unsetable = requires(T & p) {
		p.unset_elements();
	};

	enum class owned_status { not_unset, unset_started, unset_done };

	template <typename T_nest>
	struct optr_nest {

		struct bad_targets {
			struct is_target_base :
				public node_list<is_target_base>
			{
				// data
				std::exception_ptr
					m_exception;

				virtual ~is_target_base() = default;
			};

			using t_zero = node_list<is_target_base>;
			using pointer = is_target_base *;

			// data
			t_zero
				m_zero;
			t_index
				m_count;

			~bad_targets() { clear(); }

			void add(pointer p_target, std::exception_ptr p_exception) {
				p_target->m_exception = p_exception;
				m_zero.node_attach(p_target);
				++m_count;
			}

			void del(pointer p_target) {
				if( ! p_target->node_empty() ) {
					p_target->node_detach();
					--m_count;
				}
			}

			void clear() {
				while( ! m_zero.node_empty() ) { // since (delete it) might add some to ring in front
					for( typename t_zero::target_pointer it : m_zero.node_range() ) { // forward it
						it->node_detach();
						delete it;
					}
				}
				m_zero.node_reset();
				m_count = 0;
			}
		};

		static inline bad_targets s_ring;

		//

		struct junction {
			using pointer = junction *;
			using t_items = std::list<pointer>;
			using t_items_pointer = t_items *;
			using iterator = t_items::const_iterator;

			// data
			t_items
				m_sources;
		};

		struct with_point : public bad_targets::is_target_base {
			using t_ptr = std::unique_ptr<junction>;

			// data
			t_ptr
				m_point;

			with_point() : m_point{std::make_unique<junction>()} {}
			with_point(with_point &&) = default;
		};

		/*template <typename T>
		struct holder {
			using handle = T *;

			static void free(handle p_handle) { delete p_handle; }

			using t_fn_free = decltype(&free);

			// data
			bool
				m_dispose;
			handle
				m_handle;
			t_fn_free
				m_fn_free = &free;

			holder(handle p_handle) : m_dispose{false}, m_handle{p_handle} {}

			template <typename T, typename ... T_args>
			holder(in_place_type<T>, T_args && ... p_args) :
				m_dispose{true},
				m_handle{ new T{std::forward<T_args>(p_args) ...} }
			{}

			~holder() {
				if( m_dispose ) { m_fn_free(m_handle); }
			}

			handle release() {
				m_dispose = false;
				return m_handle;
			}
		};*/

		template <typename T>
		struct is_target : public with_point {
			using handle = T *;
			using t_ring = bad_targets;
			using t_ring_pointer = bad_targets *;
			//using t_holder = holder<T>;

			// data
			owned_status
				m_unset_status{owned_status::not_unset};
			t_ring_pointer
				m_ring = &s_ring;

			is_target() = default;
			is_target(is_target &&) = default;
			~is_target() { m_ring->del(this); }

			handle get_target() {
				return static_cast<handle>(this);
			}

			//virtual t_holder get_holder() { return t_holder{get_target()}; }

			void unset() {
				if( m_unset_status == owned_status::not_unset) {
					m_unset_status = owned_status::unset_started;
					static_assert(c_unsetable<T>);
					get_target()->unset_elements();
					m_unset_status = owned_status::unset_done;
				}
			}
		};

		// optr

		template <typename T>
		requires (std::derived_from<T, is_target<T> >)
		struct optr {
			using type = T;
			using pointer = type *;
			using reference = type &;
			using t_ring = bad_targets;
			using t_ring_pointer = t_ring *;

			struct params {
				// data
				pointer
					m_target;
				t_ring_pointer
					m_ring = &t_ring::s_ring;
			};

			// data
			junction::pointer
				m_source;
			junction::iterator
				m_iterator;
			pointer
				m_target{nullptr};

			template <typename ... T_args>
			optr(junction::pointer p_host, separator = {}, T_args && ... p_args) :
				m_source{p_host}
			{
				std::unique_ptr<type> v_target{ std::make_unique<type>(std::forward<T_args>(p_args) ...) };
				init(&v_target->m_point->m_sources);
				m_target = v_target.release();
			}

			~optr() {
				try {
					clear();
				} catch( ... ) {
					m_target->m_ring->add(m_target, std::current_exception() );
				}
			}

			optr(const optr & p_other) = delete;
			optr(optr && p_other) = default;

			optr(junction::pointer p_host, const optr & p_other) :
				m_source{p_host}
			{
				init(&p_other.m_target->m_point->m_sources);
				m_target = p_other.m_target;
			}

			/*optr(junction::pointer p_host, optr && p_other) :
				m_source{p_host}
			{
				std::ranges::swap(m_target, p_other.m_target);
				std::ranges::swap(m_iterator, p_other.m_iterator);
			}*/

			optr & operator = (const optr & p_other) {
				if( m_target == p_other.m_target ) { return *this; }
				clear();
				init(&p_other.m_target->m_point->m_sources);
				m_target = p_other.m_target;
				return *this;
			}

			//optr & operator = (optr && p_other) = delete;

			/*optr & operator = (optr && p_other) {
				if( m_target == p_other.m_target ) { return; }
				std::ranges::swap(m_target, p_other.m_target);
				std::ranges::swap(m_iterator, p_other.m_iterator);
				return *this;
			}*/

			reference operator * () { return *m_target; }
			pointer operator -> () { return m_target; }
			pointer get() { return m_target; }

			void clear() {
				if( m_target == nullptr ) { return; }
				typename junction::t_items_pointer v_sources = &m_target->m_point->m_sources;
				if( m_iterator != v_sources->cend() ) { v_sources->erase(m_iterator); }
				if( v_sources->empty() ) {
					delete m_target;
					m_target = nullptr;
					return;
				}
				using t_set = std::set<junction::pointer>;
				using iterator = junction::t_items::iterator;
				using t_pair = std::pair<iterator, iterator>;
				using t_list = std::list<t_pair>;
				t_set v_set;
				v_set.insert( m_target->m_point.get() );
				t_list v_list;
				v_list.emplace_back(v_sources->begin(), v_sources->end() );
				do {
					t_pair * v_pair = &v_list.back();
					if( v_pair->first == v_pair->second ) {
						v_list.pop_back();
						if( v_list.empty() ) { break; }
					} else {
						typename junction::pointer v_owner = *v_pair->first;
						if( v_owner == nullptr ) { return; }
						if( ! v_set.contains(v_owner) ) {
							v_set.insert(v_owner);
							v_list.emplace_back(v_owner->m_sources.begin(), v_owner->m_sources.end() );
						}
						++v_pair->first;
					}
				} while( true );
				// no roots found
				m_target->unset();
				m_target = nullptr;
			} // fn

		private:
			void init(junction::t_items_pointer p_hosts) {
				m_iterator = p_hosts->cend();
				p_hosts->push_front(m_source);
				m_iterator = p_hosts->cbegin();
			}
		}; // struct

		#include "just.optr.vector.h"
		#include "just.optr.map.h"
		#include "just.optr.hive.h"

	}; // struct

} // ns