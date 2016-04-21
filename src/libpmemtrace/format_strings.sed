#n
h
1{
	s|.*%\([^_]*\).*%.*$|\#include "lib\1.h"|p
	i\\#include <inttypes.h>
	i\

}

g

s/^.*%\(.*\)%.*$/static const char format_\1[] = "\1"/p

g
s/^.*(\(.*\))$/\1,/     #  fetch arguments

s/^void,$/void,/   #  special case for f(void)

s/__restrict //g        #  restrict, struct and const keywords are
s/struct //g            #  irrelevant for the format string
s/const //g
s/char \*\([a-zA-Z_]*\)/\1=\\"%s\\"/g
s/timespec \*\([a-zA-Z_]*\),/\1=%p:{.tv_sec=%" PRIdMAX ", .tv_nsec=%ld},/g
s/PMEMoid \*\([a-zA-Z_]*\),/\1=%p:{.pool_uuid_lo=0x%" PRIx64 ", .off=%" PRIu64 "},/g
s/PMEMoid \([a-zA-Z_]*\),/\1={.pool_uuid_lo=0x%" PRIx64 ", .off=%" PRIu64 "},/g
s/pmemobj_constr \([a-zA-Z_]*\),/\1=%p,/g

s/[A-Za-z_]\{1,\} \*\([a-zA-Z_]*\),/\1=%p,/g    # any pointer

s/unsigned int \([a-zA-Z_]*\),/\1=%u,/g
s/unsigned \([a-zA-Z_]*\),/\1=%u,/g
s/int \([a-zA-Z_]*\),/\1=%d,/g
s/enum [a-zA-Z_]\{1,\} \([a-zA-Z_]*\),/\1=%d,/g
s/unsigned long \([a-zA-Z_]*\),/\1=%lu,/g
s/long \([a-zA-Z_]*\),/\1=%l,/g
s/mode_t \([a-zA-Z_]*\),/\1=%d,/g
s/size_t \([a-zA-Z_]*\),/\1=%zu,/g
s/uint64_t \([a-zA-Z_]*\),/\1=%" PRIu64 ",/g
s/^/\t"(/
s/,$/)"/
p

:print_return_type
g

s/[ ]*%.*%.*$//

s/^void$/" = (void)";/
s/^PMEMoid$/" = {.pool_uuid_lo=0x%" PRIx64 ", .off=%" PRIu64 "}";/
s/^const char \*$/" = \\"%s\\"";/
s/^[a-zA-Z_][a-z_A-Z0-9]*[ ]*\*$/" = %p";/
s/const //g
s/^unsigned int$/" = %u";/
s/^int$/" = %d";/
s/^mode_t$/" = %d";/
s/^size_t$/" = %zu";/
s/^PMEMoid$/" = {.pool_uuid_lo=0x%" PRIx64 ", .off=%" PRIu64 "}";/
s/^uint64_t$/" = %" PRIu64 "";/
s/^[a-zA-Z_][a-z_A-Z0-9]*[ ]*\*$/" = %p";/
s/^/	/
p

i\

