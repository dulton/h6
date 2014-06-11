#ifndef _PROTO_PARSER_H_
#define _PROTO_PARSER_H_

#include "msg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __proto_parser       proto_parser;
typedef struct __proto_parser_ops   proto_parser_ops;


struct __proto_parser
{
	obj_t	            obj_base;

	proto_parser_ops    *ops;
	void                *private;
};


struct __proto_parser_ops
{
	int32_t (*init)(proto_parser *p, void *u);
	void	(*fin)(proto_parser *p);

	msg_t *(*parse)(proto_parser *p, uint8_t *data, uint32_t len, int32_t *err);
	msg_t *(*parse_io)(proto_parser *p, void *io, int32_t *err);
};


proto_parser *alloc_proto_parser(uint32_t size, proto_parser_ops *ops, void *u);
void free_proto_parser(proto_parser *parser);

msg_t *parse_proto(proto_parser *p, uint8_t *data, uint32_t len, int32_t *err);
msg_t *parse_proto_io(proto_parser *p, void *io, int32_t *err);

#ifdef __cplusplus
}
#endif

#endif
