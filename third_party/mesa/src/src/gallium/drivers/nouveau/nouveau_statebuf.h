#ifndef __NOUVEAU_STATEBUF_H__
#define __NOUVEAU_STATEBUF_H__

/* state buffers: lightweight state objects interface */
/* relocations are not supported, but Gallium CSOs don't require them */

struct nouveau_statebuf_builder
{
	uint32_t* p;
#ifdef DEBUG
	uint32_t* pend;
#endif
};

#ifdef DEBUG
#define sb_init(var) {var, var + sizeof(var) / sizeof((var)[0])}
#define sb_data(sb, v) do {assert((sb).p != (sb).pend);  *(sb).p++ = (v);} while(0)
#else
#define sb_init(var) {var}
#define sb_data(sb, v) *(sb).p++ = (v)
#endif

static INLINE uint32_t sb_header(unsigned subc, unsigned mthd, unsigned size)
{
	return (size << 18) | (subc << 13) | mthd;
}

#define sb_method(sb, v, n)  sb_data(sb, sb_header(SUBC_3D(v), n));

#define sb_len(sb, var) ((sb).p - (var))
#define sb_emit(push, sb_buf, sb_len) do {PUSH_SPACE((push), (sb_len)); PUSH_DATAp((push), (sb_buf), (sb_len)); } while(0)
#endif
