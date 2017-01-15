#!/bin/bash
#
# Copyright 2017, Intel Corporation
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#
#     * Neither the name of the copyright holder nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

symbols_file=$(mktemp)
NMEDIT=$1
shift
prev_is_dash_g="0"
input_path=""
output_path=""

for a in $@
do
	if [ "$a" == "-G" ]; then
		prev_is_dash_g="1"
	else
		if [ "$prev_is_dash_g" == "1" ]; then
			if [ "$a" != "pmemobj_mutex_timedlock" -a \
				"$a" != "pmemobj_rwlock_timedrdlock" -a \
				"$a" != "pmemobj_rwlock_timedwrlock" -a \
				"$a" != "__free_hook" -a \
				"$a" != "__malloc_hook" -a \
				"$a" != "__memalign_hook" -a \
				"$a" != "__realloc_hook" -a \
				"$a" != "memalign" -a \
				"$a" != "pvalloc" -a \
				"$a" != "valloc" ]; then
				echo _$a >> $symbols_file
			fi
		elif [ "$input_path" == "" ]; then
			input_path="$a"
		elif [ "$output_path" == "" ]; then
			output_path="$a"
		else
			echo error
			exit 1
		fi
		prev_is_dash_g="0"
	fi
done

$NMEDIT -s $symbols_file $input_path -o $output_path
