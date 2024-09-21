#pragma once

#include <initializer_list>

namespace RS
{
	template<typename Type, uint32 N>
	struct VectorSwizzlingDataBase
	{
		Type values[N];
	};

	template<typename Type>
	struct VectorSwizzlingDataBase<Type, 1>
	{
		union
		{
			Type values[1];
			union
			{
				Type x;
				Type r;
				Type u;
			};
		};
	};

	template<typename Type>
	struct VectorSwizzlingDataBase<Type, 2>
	{
		union
		{
			Type values[2];
			struct
			{
				union
				{
					Type x;
					Type r;
					Type u;
				};
				union
				{
					Type y;
					Type g;
					Type v;
				};
			};
		};
	};

	template<typename Type>
	struct VectorSwizzlingDataBase<Type, 3>
	{
		union
		{
			Type values[3];
			struct
			{
				union
				{
					Type x;
					Type r;
					Type u;
				};
				union
				{
					Type y;
					Type g;
					Type v;
				};
				union
				{
					Type z;
					Type b;
					Type s;
				};
			};
		};
	};

	template<typename Type>
	struct VectorSwizzlingDataBase<Type, 4>
	{
		union
		{
			Type values[4];
			struct
			{
				union
				{
					Type x;
					Type r;
					Type u;
				};
				union
				{
					Type y;
					Type g;
					Type v;
				};
				union
				{
					Type z;
					Type b;
					Type s;
				};
				union
				{
					Type w;
					Type a;
					Type t;
				};
			};
		};
	};

	template<typename Type, uint32 N>
	struct VectorSwizzlingData : public VectorSwizzlingDataBase<Type, N>
	{
		VectorSwizzlingData(bool initialize, Type value = static_cast<Type>(0));
		VectorSwizzlingData(const std::initializer_list<Type>& list);
	};

	template<typename Type, uint32 N>
	inline VectorSwizzlingData<Type, N>::VectorSwizzlingData(bool initialize, Type value)
	{
		for (uint8 i = 0; initialize && i < N; ++i)
			this->values[i] = value;
	}

	template<typename Type, uint32 N>
	inline VectorSwizzlingData<Type, N>::VectorSwizzlingData(const std::initializer_list<Type>& list)
	{
		RS_ASSERT(list.size() == N, "Initializer list size does not match the VectorSwizzlingData size!");

		// TODO: Might be able to use memcpy if initializer_list guarantees packed allication (not sparse).
		uint8 i = 0;
		for (auto it = list.begin(); it != list.end(); it++)
			this->values[i++] = *it;
	}
}