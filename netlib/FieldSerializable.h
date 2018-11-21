#ifndef FIELD_SERIALIZABLE_H_
#define FIELD_SERIALIZABLE_H_

#include "commlib.h"

class FieldSerializable {
	virtual bool field_serialize(netfields &nfields) = 0;
	virtual bool field_deserialize(netfields &nfields) = 0;
};

#endif