#ifndef YAML_EMITVECTOR_H
#define YAML_EMITVECTOR_H

#include <vector>

namespace YAML
{
	template <typename T>
	inline Emitter& operator << (Emitter& emitter, const std::vector <T>& v) {
		typedef typename std::vector <T> vec;
		emitter << BeginSeq;
		for(typename vec::const_iterator it=v.begin();it!=v.end();++it)
			emitter << *it;
		emitter << EndSeq;
		return emitter;
	}	
}

#endif
