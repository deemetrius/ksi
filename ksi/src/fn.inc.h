#pragma once
#include "space.h"

namespace ksi {
namespace native {

#define UNEXPECTED(expr) (expr)

// for array

template <class T, class Cmp>
struct t_cmp_array {
	T begin_;
	id * pos_;
	inline id operator () (const T r1, const T r2) const {
		id ret = Cmp::compare(r1, r2);
		if( !ret )
		ret = ex::cmp::special_compare(pos_[r1 - begin_], pos_[r2 - begin_]);
		return ret;
	}
};

template <class T>
struct t_swp_array {
	T begin_;
	id * pos_;
	inline void operator () (T r1, T r2) const {
		r1->swap(*r2);
		ex::swap(pos_[r1 - begin_], pos_[r2 - begin_]);
	}
};

// for map

struct t_map_pair {
	using cnode = var::keep_map::t_items::cnode;
	cnode * node_;
	id pos_;
};

template <class Cmp>
struct t_cmp_map {
	inline id operator () (const t_map_pair * r1, const t_map_pair * r2) const {
		id ret = Cmp::compare(&r1->node_->val_, &r2->node_->val_);
		if( !ret )
		ret = ex::cmp::special_compare(r1->pos_, r2->pos_);
		return ret;
	}
};

struct t_swp_map {
	inline void operator () (t_map_pair * r1, t_map_pair * r2) const {
		ex::swap(*r1, *r2);
	}
};

// sort

struct t_sort {
	/* based on zend_sort (from php 8) */

	template <class T, class Cmp, class Swp>
	static inline void sort_2(T * a, T * b, const Cmp & cmp, const Swp & swp) {
		if( cmp(a, b) > 0 ) {
			swp(a, b);
		}
	}

	template <class T, class Cmp, class Swp>
	static inline void sort_3(T * a, T * b, T * c, const Cmp & cmp, const Swp & swp) {
		if( !(cmp(a, b) > 0) ) {
			if( !(cmp(b, c) > 0) ) {
				return;
			}
			swp(b, c);
			if( cmp(a, b) > 0 ) {
				swp(a, b);
			}
			return;
		}
		if( !(cmp(c, b) > 0) ) {
			swp(a, c);
			return;
		}
		swp(a, b);
		if( cmp(b, c) > 0 ) {
			swp(b, c);
		}
	}

	template <class T, class Cmp, class Swp>
	static void sort_4(T * a, T * b, T * c, T * d, const Cmp & cmp, const Swp & swp) {
		sort_3(a, b, c, cmp, swp);
		if( cmp(c, d) > 0 ) {
			swp(c, d);
			if( cmp(b, c) > 0 ) {
				swp(b, c);
				if( cmp(a, b) > 0 ) {
					swp(a, b);
				}
			}
		}
	}

	template <class T, class Cmp, class Swp>
	static void sort_5(T * a, T * b, T * c, T * d, T * e, const Cmp & cmp, const Swp & swp) {
		sort_4(a, b, c, d, cmp, swp);
		if( cmp(d, e) > 0 ) {
			swp(d, e);
			if( cmp(c, d) > 0 ) {
				swp(c, d);
				if( cmp(b, c) > 0 ) {
					swp(b, c);
					if( cmp(a, b) > 0 ) {
						swp(a, b);
					}
				}
			}
		}
	}

	template <class T, class Cmp, class Swp>
	static void insert_sort(T * base, id nmemb, const Cmp & cmp, const Swp & swp) {
		switch( nmemb ) {
		case 0:
		case 1:
			break;
		case 2:
			sort_2(base, base +1, cmp, swp);
			break;
		case 3:
			sort_3(base, base +1, base +2, cmp, swp);
			break;
		case 4:
			sort_4(base, base +1, base + 2, base +3, cmp, swp);
			break;
		case 5:
			sort_5(base, base +1, base + 2, base +3, base +4, cmp, swp);
			break;
		default:
			{
				T *i, *j, *k;
				T *start = base;
				T *end = start + nmemb;
				T *sentry = start +6;
				for( i = start + 1; i < sentry; i += 1 ) {
					j = i - 1;
					if( !(cmp(j, i) > 0) ) {
						continue;
					}
					while( j != start ) {
						j -= 1;
						if( !(cmp(j, i) > 0) ) {
							j += 1;
							break;
						}
					}
					for( k = i; k > j; k -= 1 ) {
						swp(k, k - 1);
					}
				}
				for( i = sentry; i < end; i += 1 ) {
					j = i - 1;
					if( !(cmp(j, i) > 0) ) {
						continue;
					}
					do {
						j -= 2;
						if( !(cmp(j, i) > 0) ) {
							j += 1;
							if( !(cmp(j, i) > 0) ) {
								j += 1;
							}
							break;
						}
						if( j == start ) {
							break;
						}
						if( j == start + 1 ) {
							j -= 1;
							if( cmp(i, j) > 0 ) {
								j += 1;
							}
							break;
						}
					} while( true );
					for( k = i; k > j; k -= 1 ) {
						swp(k, k - 1);
					}
				}
			}
			break;
		}
	}

	template <class T, class Cmp, class Swp>
	static void sort(T * base, id nmemb, const Cmp & cmp, const Swp & swp) {
		while( true ) {
			if (nmemb <= 16) {
				insert_sort(base, nmemb, cmp, swp);
				return;
			} else {
				T *i, *j;
				T *start = base;
				T *end = start + nmemb;
				id offset = nmemb >> 1;
				T *pivot = start + offset;

				if( nmemb >> 10 ) {
					id delta = offset >> 1;
					sort_5(start, start + delta, pivot, pivot + delta, end - 1, cmp, swp);
				} else {
					sort_3(start, pivot, end - 1, cmp, swp);
				}
				swp(start + 1, pivot);
				pivot = start + 1;
				i = pivot + 1;
				j = end - 1;
				while( true ) {
					while( cmp(pivot, i) > 0 ) {
						i += 1;
						if( UNEXPECTED(i == j) ) {
							goto done;
						}
					}
					j -= 1;
					if( UNEXPECTED(j == i) ) {
						goto done;
					}
					while( cmp(j, pivot) > 0 ) {
						j -= 1;
						if( UNEXPECTED(j == i) ) {
							goto done;
						}
					}
					swp(i, j);
					i += 1;
					if( UNEXPECTED(i == j) ) {
						goto done;
					}
				}
	done:
				swp(pivot, i -1);
				if( (i -1) - start < end - i ) {
					sort(start, (i - start) -1, cmp, swp);
					base = i;
					nmemb = end - i;
				} else {
					sort(i, end - i, cmp, swp);
					nmemb = i - start -1;
				}
			}
		}
	}
};

} // ns
} // ns
