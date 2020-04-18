
int init_rc_normal_src(
	struct rc_instruction * inst,
	unsigned int src_index,
	const char * src_str);

int init_rc_normal_dst(
	struct rc_instruction * inst,
	const char * dst_str);

int init_rc_normal_instruction(
	struct rc_instruction * inst,
	const char * inst_str);
