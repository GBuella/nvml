#n
h
1{
	i\\#include "libpmemtrace.h"
	s|.*%\([^_]*\).*%.*$|\#include "\1_formats.h"|p
	i\

}

g

s/%//gp # print the function prototype verbatim

i\{

g
# generate a pointer to the original functions
s/%.*%/(*forward)/
s/$/;/
s/^/\tstatic /p


# generate a variable to hold the return value,
# if need be
g
/^void %/!{
	s|^\(.*\)%.*%.*$|\t\1result;|p
}

i\

i\
	pmemtrace_setup_forward((void**)&forward, __func__);

g
s|^.*%\(.*\)%.*$|\#ifdef HOOK_\1|p
g
/^void %/!{
	s|^[^%]*%|\tresult = HOOK_|
}
/^void %/{
	s|^[^%]*%|\tHOOK_|
}
s|)|,);|
s|[^(,]*[ \*]\([a-z_A-Z0-9]*\),|\1, |g
s|, )|, forward)|
s|(void,)|(forward)|
s|%||g
p

i\\#else

g
/^void %/!{
	s|^.*%.*%|\tresult = forward|
}
/^void %/{
	s|^.*%.*%|\tforward|
}
s|)|,);|
s|[^(,]*[ \*]\([a-z_A-Z0-9]*\),|\1, |g
s|, )|)|
s|(void,)|()|
s|%||g
p

i\\#endif

g
s|%|pmemtrace_log(format_|
s|%(|, |
s|)|, |

s/, void,/,/

s/__restrict //g             # remove unneeded
s/struct //g                 # tokens, keep only
s/const //g                  # a "type name" form

# a few types get special treatment
s/PMEMoid \*\([a-zA-Z_0-9]*\), /(const void*)\1, \1 ? \1->pool_uuid_lo : 0, \1 ? \1->off : 0, /g
s/PMEMoid \([a-zA-Z_0-9]*\), /\1.pool_uuid_lo, \1.off, /g
s/timespec \*\([a-zA-Z_0-9]*\), /(const void*)\1, \1 ? \1->tv_sec : 0, \1 ? \1->tv_nsec : 0, /g

# %s expects char pointer
s/char \*\([a-zA-Z_0-9]*\), /\1, /g

# but %p expects void pointer
s/[^, :]\{1,\} \*\([a-zA-Z_0-9]*\), /(const void*)\1, /g

# non pointer type, just use the variable name
s/enum [^, :]\{1,\} \([a-zA-Z_0-9]*\), /\1, /g
s/[^, :]\{1,\} \([a-zA-Z_0-9]*\), /\1, /g


# handle the return type

/^void pmemtrace_log(/{
		s/, $/);/
}
/^PMEMoid pmemtrace_log(/{
		s/, $/, result.pool_uuid_lo, result.off);/
}
/^void pmemtrace_log(/!{
		s/, $/, result);/
}
s/^.*pmemtrace_log(/\tpmemtrace_log(/

p

g

/pop, PMEMoid \*oidp,/{
	i\
	if (oidp != NULL)
	i\
		pmemtrace_oid_create(*oidp);
}


g
/^void %/!{
	i\\treturn result;
}

i\}
i\

