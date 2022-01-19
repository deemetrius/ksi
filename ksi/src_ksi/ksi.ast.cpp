module;

export module ksi.ast;

export namespace ksi::ast {

// precedence
enum n_prec {
	prec_leaf,
	prec_then_after,		//	? `then-end loops a$
	prec_member,			//	a.key a[key]
	prec_type_of_before,	//	a$
	prec_rt_assign_after,	//	=>
	prec_assign_before,		//	=
	prec_invoke,			//	! &				(fn call by ref, fn get ref)
	prec_mult,				//	* / `mod
	prec_and_bits,			//	& `_and			(bitwise)
	prec_xor_bits,			//	^ `_xor
	prec_or_bits,			//	| `_or
	prec_plus,				//	+ - % %% #std_fn &user_fn
	prec_cmp,				//	<=>
	prec_eq,				//	== <> < <= > >=
	prec_throw,				//	`throw
	prec_and,				//	`and			(logical)
	prec_xor,				//	`xor
	prec_or,				//	`or
	prec_nullc,				//	?? ?!			(null coalescing)
	prec_assign_after,		//	=
	prec_rt_assign_before,	//	=>
	prec_then_before,		//	? `then-end loops
	prec_pair,				//	:				(key-value pair)
	prec_root
};

} // ns ksi::ast