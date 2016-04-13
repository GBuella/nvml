#n

# assemble multi line function declarations into the pattern space
:args
/,$/{N;bargs}
/($/{N;bargs}

# pmemobj_tx_begin uses varargs, which is not that easy to hook
/pmemobj_tx_begin/d

# skip the pmemobj_set_funcs function,
# so we dont have to deal with parsing function
# pointer arguments
/pmemobj_set_funcs/d

/^\(const \)\{,1\}\(struct \)\{,1\}[a-zA-Z0-9_]* \*\{0,1\}pmem[a-z]*_[a-z_0-9]*([^)]*);/{
	s/).*;.*$/)/
	y/\t/ /
	y/\n/ /
	s/^ *//
	s/[ ]\{1,\}/ /g
	s/ ,//
	s/( /(/
	s/\(pmem[a-z]*_[a-z_0-9]*\)(/%\1%(/
	p
}
