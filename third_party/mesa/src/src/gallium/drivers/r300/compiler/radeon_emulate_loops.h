

#ifndef RADEON_EMULATE_LOOPS_H
#define RADEON_EMULATE_LOOPS_H

#define MAX_ITERATIONS 8

struct radeon_compiler;

struct loop_info {
	struct rc_instruction * BeginLoop;
	struct rc_instruction * Cond;
	struct rc_instruction * If;
	struct rc_instruction * Brk;
	struct rc_instruction * EndIf;
	struct rc_instruction * EndLoop;
};

struct emulate_loop_state {
	struct radeon_compiler * C;
	struct loop_info * Loops;
	unsigned int LoopCount;
	unsigned int LoopReserved;
};

void rc_transform_loops(struct radeon_compiler *c, void *user);

void rc_unroll_loops(struct radeon_compiler * c, void *user);

void rc_emulate_loops(struct radeon_compiler * c, void *user);

#endif /* RADEON_EMULATE_LOOPS_H */
