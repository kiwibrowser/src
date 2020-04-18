/**************************************************************************
 *
 * Copyright 2010 Luca Barbieri
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include <vector>
#include <set>
#include "sm4.h"

#define check(x) do {if(!(x)) return false;} while(0)

bool sm4_link_cf_insns(sm4_program& program)
{
	if(program.cf_insn_linked.size())
		return true;

	std::vector<int> cf_insn_linked;
	cf_insn_linked.resize(program.insns.size());
	memset(&cf_insn_linked[0], 0xff, cf_insn_linked.size() * sizeof(int));
	std::vector<unsigned> cf_stack;
	for(unsigned insn_num = 0; insn_num < program.insns.size(); ++insn_num)
	{
		unsigned v;
		switch(program.insns[insn_num]->opcode)
		{
		case SM4_OPCODE_LOOP:
			cf_stack.push_back(insn_num);
			break;
		case SM4_OPCODE_ENDLOOP:
			check(!cf_stack.empty());
			v = cf_stack.back();
			check(program.insns[v]->opcode == SM4_OPCODE_LOOP);
			cf_insn_linked[v] = insn_num;
			cf_insn_linked[insn_num] = v;
			cf_stack.pop_back();
			break;
		case SM4_OPCODE_IF:
		case SM4_OPCODE_SWITCH:
			cf_insn_linked[insn_num] = insn_num; // later changed
			cf_stack.push_back(insn_num);
			break;
		case SM4_OPCODE_ELSE:
		case SM4_OPCODE_CASE:
			check(!cf_stack.empty());
			v = cf_stack.back();
			if(program.insns[insn_num]->opcode == SM4_OPCODE_ELSE)
				check(program.insns[v]->opcode == SM4_OPCODE_IF);
			else
				check(program.insns[v]->opcode == SM4_OPCODE_SWITCH || program.insns[v]->opcode == SM4_OPCODE_CASE);
			cf_insn_linked[insn_num] = cf_insn_linked[v]; // later changed
			cf_insn_linked[v] = insn_num;
			cf_stack.back() = insn_num;
			break;
		case SM4_OPCODE_ENDSWITCH:
		case SM4_OPCODE_ENDIF:
			check(!cf_stack.empty());
			v = cf_stack.back();
			if(program.insns[insn_num]->opcode == SM4_OPCODE_ENDIF)
				check(program.insns[v]->opcode == SM4_OPCODE_IF || program.insns[v]->opcode == SM4_OPCODE_ELSE);
			else
				check(program.insns[v]->opcode == SM4_OPCODE_SWITCH || program.insns[v]->opcode == SM4_OPCODE_CASE);
			cf_insn_linked[insn_num] = cf_insn_linked[v];
			cf_insn_linked[v] = insn_num;
			cf_stack.pop_back();
			break;
		}
	}
	check(cf_stack.empty());
	program.cf_insn_linked.swap(cf_insn_linked);
	return true;
}

bool sm4_find_labels(sm4_program& program)
{
	if(program.labels_found)
		return true;

	std::vector<int> labels;
	for(unsigned insn_num = 0; insn_num < program.insns.size(); ++insn_num)
	{
		switch(program.insns[insn_num]->opcode)
		{
		case SM4_OPCODE_LABEL:
			if(program.insns[insn_num]->num_ops > 0)
			{
				sm4_op& op = *program.insns[insn_num]->ops[0];
				if(op.file == SM4_FILE_LABEL && op.has_simple_index())
				{
					unsigned idx = (unsigned)op.indices[0].disp;
					if(idx >= labels.size())
						labels.resize(idx + 1);
					labels[idx] = insn_num;
				}
			}
			break;
		}
	}
	program.label_to_insn_num.swap(labels);
	program.labels_found = true;
	return true;
}
