module;

#include "../src/pre.h"

export module just.owned_ptr;

export import <type_traits>;
export import <concepts>;
export import <list>;
export import <exception>;
import <memory>;
import <utility>;
import <set>;

export namespace just {

	struct owner {
		using pointer = owner *;
		using t_items = std::list<pointer>;
		using t_items_pointer = t_items *;
		using iterator = t_items::const_iterator;

		// data
		t_items		m_hosts;
	};

	template <typename T>
	concept c_unsetable = requires(T & p) {
		p.unset_elements();
	};

	enum class owned_status { not_unset, unset_started, unset_done };

	template <c_unsetable T>
	struct is_owned {
		// data
		owner			m_owner;
		owned_status	m_unset_status{owned_status::not_unset};

		void unset() {
			if( m_unset_status == owned_status::not_unset) {
				m_unset_status = owned_status::unset_started;
				static_cast<T *>(this)->unset_elements();
				m_unset_status = owned_status::unset_done;
			}
		}
	};

	template <typename T>
	concept c_owned = std::derived_from<T, is_owned<T> >;

	template <c_owned T>
	struct owned_ptr {
		using type = T;
		using pointer = type *;
		using reference = type &;
		
		struct exception_state {
			// data
			pointer				m_target{nullptr};
			std::exception_ptr	m_exception;
		};

		static exception_state s_state;

		// data
		owner::pointer		m_host;
		owner::iterator		m_iterator;
		pointer				m_target{nullptr};

		template <typename ... T_args>
		owned_ptr(owner::pointer p_host, T_args && ... p_args) :
			m_host{p_host}
		{
			std::unique_ptr<type> v_target{ std::make_unique<type>(std::forward<T_args>(p_args) ...) };
			init(&v_target->m_owner.m_hosts);
			m_target = v_target.release();
		}

		~owned_ptr() {
			try {
				clear();
			} catch( ... ) {
				s_state.m_target = m_target;
				s_state.m_exception = std::current_exception();
			}
		}

		owned_ptr(const owned_ptr & p_other) = delete;
		owned_ptr(owned_ptr && p_other) = delete;

		owned_ptr(owner::pointer p_host, const owned_ptr & p_other) :
			m_host{p_host}
		{
			init(&p_other.m_target->m_owner.m_hosts);
			m_target = p_other.m_target;
		}

		owned_ptr(owner::pointer p_host, owned_ptr && p_other) :
			m_host{p_host}
		{
			std::ranges::swap(m_target, p_other.m_target);
			std::ranges::swap(m_iterator, p_other.m_iterator);
		}

		owned_ptr & operator = (const owned_ptr & p_other) {
			if( m_target == p_other.m_target ) { return; }
			clear();
			init(&p_other.m_target->m_owner.m_hosts);
			m_target = p_other.m_target;
			return *this;
		}

		owned_ptr & operator = (owned_ptr && p_other) {
			if( m_target == p_other.m_target ) { return; }
			std::ranges::swap(m_target, p_other.m_target);
			std::ranges::swap(m_iterator, p_other.m_iterator);
			return *this;
		}

		reference operator * () { return *m_target; }
		pointer operator -> () { return m_target; }

	private:
		void init(owner::t_items_pointer p_hosts) {
			m_iterator = p_hosts->cend();
			p_hosts->push_front(m_host);
			m_iterator = p_hosts->cbegin();
		}

		void clear() {
			if( m_target == nullptr ) { return; }
			owner::t_items_pointer v_hosts = &m_target->m_owner.m_hosts;
			if( m_iterator != v_hosts->cend() ) { v_hosts->erase(m_iterator); }
			if( v_hosts->empty() ) {
				delete m_target;
				m_target = nullptr;
				return;
			}
			using t_set = std::set<owner::pointer>;
			using iterator = owner::t_items::iterator;
			using t_pair = std::pair<iterator, iterator>;
			using t_list = std::list<t_pair>;
			t_set v_set;
			v_set.insert(&m_target->m_owner);
			t_list v_list;
			v_list.emplace_back(v_hosts->begin(), v_hosts->end() );
			do {
				t_pair * v_pair = &v_list.back();
				if( v_pair->first == v_pair->second ) {
					v_list.pop_back();
					if( v_list->empty() ) { break; }
				} else {
					owner::pointer v_owner = *v_pair.first;
					if( v_owner == nullptr ) { return; }
					if( ! v_set.contains(v_owner) ) {
						v_set.insert(v_owner);
						v_list.emplace_back(v_owner->m_hosts.begin(), v_owner->m_hosts.end() );
					}
					++v_pair.first;
				}
			} while( true );
			// no roots found
			m_target->unset();
			m_target = nullptr;
		} // fn
	}; // struct

} // ns