module;

export module just.keeper;
export import just.common;
import <concepts>;
import <type_traits>;

export namespace just {

namespace detail {

template <typename Base, uid Size, uid Align>
struct alignas(Align) keeper {
	using type = Base;
	using pointer = type *;
	enum : uid {
		local_size = Size,
		local_align = Align
	};

	// data
	alignas(local_align)
	aligned_data<local_size, local_align> place;
	pointer handle = nullptr;

	keeper & reset() { handle = nullptr; return *this; }
	void assign(pointer h) { handle = h; }

	operator bool () const { return handle; }
	bool operator ! () const { return !handle; }
	pointer operator -> () const { return handle; }
};

template <typename Base, uid Size, uid Align>
struct keeper_special :
	public keeper<Base, Size, Align>
{
	using self_base = keeper<Base, Size, Align>;
	using type = self_base::type;
	using pointer = self_base::pointer;

	~keeper_special() { close(); }

	keeper_special & reset() {
		if( this->handle ) { this->handle->~type(); this->handle = nullptr; }
		return *this;
	}

private:
	inline void close() { if( this->handle ) this->handle->~type(); }
};

} // ns detail

template <typename Base, std::derived_from<Base> ... Options>
struct keeper {
	using type = Base;
	static constexpr bool is_only = (sizeof...(Options) == 0);
	enum : uid {
		local_size = is_only ? sizeof(type) : max(uid{}, sizeof(Options) ...),
		local_align = is_only ? alignof(type) : max(uid{}, alignof(Options) ...)
	};
	static constexpr bool is_special = is_only ?
		!std::is_trivially_destructible_v<type> :
		max(false, !std::is_trivially_destructible_v<Options> ...)
	;
	using t_internal = std::conditional_t<is_special,
		detail::keeper_special<type, local_size, local_align>,
		detail::keeper<type, local_size, local_align>
	>;
};

} // ns just