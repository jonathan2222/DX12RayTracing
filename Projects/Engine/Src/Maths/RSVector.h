#pragma once

// TODO: Remove as much inheritance as possible!
//		It was made like this such that it would be easier to implement, but it is slower in practice because of the amount of v-tables.

#include "RSVectorDataSwizzling.h"
#include "Utils/Misc/StringUtils.h" // Utils::Format

namespace RS
{
	template <typename Type, uint32 N, typename SwizzlingData = VectorSwizzlingData<Type, N>>
	struct Vec : public SwizzlingData
	{
		Vec();
		Vec(const std::initializer_list<Type>& list);
		Vec(const Vec& other);
		Vec(const Vec&& other);

		template<typename... Types>
		Vec(Type type, Types... types);
		template<typename MType, uint32 M, typename... Types>
		Vec(const Vec<MType, M, VectorSwizzlingData<MType, M>>& other, Types... types);

		template<uint32 M>
		bool operator==(const Vec<Type, M, VectorSwizzlingData<Type, M>>& other) const;
		template<uint32 M>
		bool operator!=(const Vec<Type, M, VectorSwizzlingData<Type, M>>& other) const;

		bool operator==(const std::initializer_list<Type>& list) const;
		bool operator!=(const std::initializer_list<Type>& list) const;

		Vec& operator=(const Type& scalar);
		Vec& operator=(const std::initializer_list<Type>& list);
		Vec& operator=(const Vec& other);
		Vec& operator=(const Vec&& other);

		Vec operator+(const Vec& other) const;
		Vec operator-(const Vec& other) const;
		Vec operator+(const Type& scalar) const;
		Vec operator-(const Type& scalar) const;
		Vec operator*(const Type& scalar) const;
		Vec operator/(const Type& scalar) const;

		Vec& operator+=(const Vec& other);
		Vec& operator-=(const Vec& other);
		Vec& operator+=(const Type& scalar);
		Vec& operator-=(const Type& scalar);
		Vec& operator*=(const Type& scalar);
		Vec& operator/=(const Type& scalar);

		Type& operator[](uint32 index);
		Type operator[](uint32 index) const;
		Type& At(uint32 index);
		Type AtConst(uint32 index) const;

		Type Dot(const Vec& other) const;

		void Copy(const Vec& other);
		void Move(const Vec&& other);

		static const Vec<Type, N, SwizzlingData> ZERO;

		std::string ToString() const;
		operator std::string() const;

	private:
		template<typename... Types>
		void Create(uint32 n, Type type, Types... types);

		void Create(uint32 n);
	};

	template<typename Type, uint32 N, typename SwizzlingData>
	const Vec<Type, N, SwizzlingData> Vec<Type, N, SwizzlingData>::ZERO = Vec<Type, N, SwizzlingData>( static_cast<Type>(0) );

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData>::Vec()
		: SwizzlingData(true, static_cast<Type>(0))
	{
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData>::Vec(const std::initializer_list<Type>& list)
		: SwizzlingData(list)
	{
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData>::Vec(const Vec& other)
		: SwizzlingData(false)
	{
		Copy(other);
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData>::Vec(const Vec&& other)
		: SwizzlingData(false)
	{
		Move(std::move(other));
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	template<typename ...Types>
	inline Vec<Type, N, SwizzlingData>::Vec(Type type, Types ...types)
		: SwizzlingData(false)
	{
		Create(0u, type, types...);
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	template<typename MType, uint32 M, typename ...Types>
	inline Vec<Type, N, SwizzlingData>::Vec(const Vec<MType, M, VectorSwizzlingData<MType, M>>& other, Types ...types)
		: SwizzlingData(false)
	{
		for (uint32 i = 0; i < std::min(N, M); ++i)
			values[i] = static_cast<Type>(other.values[i]);

		if (M < N)
			Create(M, types...);
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	template<uint32 M>
	inline bool Vec<Type, N, SwizzlingData>::operator==(const Vec<Type, M, VectorSwizzlingData<Type, M>>& other) const
	{
		if (N != M) return false;
		for (uint32 i = 0; i < N; ++i)
		{
			if (std::abs(values[i++] - other.values[i]) > static_cast<Type>(FLT_EPSILON))
				return false;
		}
		return true;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	template<uint32 M>
	inline bool Vec<Type, N, SwizzlingData>::operator!=(const Vec<Type, M, VectorSwizzlingData<Type, M>>& other) const
	{
		return !(*this == other);
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline bool Vec<Type, N, SwizzlingData>::operator==(const std::initializer_list<Type>& list) const
	{
		if (list.size() != N)
			return false;

		uint32 i = 0;
		for (std::initializer_list<Type>::iterator it = list.begin(); it != list.end(); it++)
		{
			if (std::abs(values[i++] - *it) > static_cast<Type>(FLT_EPSILON))
				return false;
		}
		return true;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline bool Vec<Type, N, SwizzlingData>::operator!=(const std::initializer_list<Type>& list) const
	{
		return !(*this == list);
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData>& Vec<Type, N, SwizzlingData>::operator=(const Type& scalar)
	{
		for (uint8 i = 0; i < N; ++i)
			values[i] = scalar;
		return *this;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData>& Vec<Type, N, SwizzlingData>::operator=(const std::initializer_list<Type>& list)
	{
		RS_ASSERT(list.size() > 0, "List must have at least one element!");
		uint32 i = 0;
		for (std::initializer_list<Type>::iterator it = list.begin(); it != list.end() && i < N; it++)
			values[i++] = *it;
		for (uint32 j = i; j < N; ++j)
			values[j] = values[i - 1];
		return *this;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData>& Vec<Type, N, SwizzlingData>::operator=(const Vec& other)
	{
		Copy(other);
		return *this;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData>& Vec<Type, N, SwizzlingData>::operator=(const Vec&& other)
	{
		Move(other);
		return *this;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData> Vec<Type, N, SwizzlingData>::operator+(const Vec& other) const
	{
		Vec result(this);
		result += other;
		return result;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData> Vec<Type, N, SwizzlingData>::operator-(const Vec& other) const
	{
		Vec result(this);
		result -= other;
		return result;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData> Vec<Type, N, SwizzlingData>::operator+(const Type& scalar) const
	{
		Vec result(this);
		result += other;
		return result;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData> Vec<Type, N, SwizzlingData>::operator-(const Type& scalar) const
	{
		Vec result(this);
		result -= other;
		return result;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData> Vec<Type, N, SwizzlingData>::operator*(const Type& scalar) const
	{
		Vec result(this);
		result *= other;
		return result;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData> Vec<Type, N, SwizzlingData>::operator/(const Type& scalar) const
	{
		Vec result(this);
		result /= other;
		return result;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData>& Vec<Type, N, SwizzlingData>::operator+=(const Vec& other)
	{
		for (uint8 i = 0; i < N; ++i)
			values[i] += other.values[i];
		return *this;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData>& Vec<Type, N, SwizzlingData>::operator-=(const Vec& other)
	{
		for (uint8 i = 0; i < N; ++i)
			values[i] -= other.values[i];
		return *this;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData>& Vec<Type, N, SwizzlingData>::operator+=(const Type& scalar)
	{
		for (uint8 i = 0; i < N; ++i)
			values[i] += scalar;
		return *this;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData>& Vec<Type, N, SwizzlingData>::operator-=(const Type& scalar)
	{
		for (uint8 i = 0; i < N; ++i)
			values[i] -= scalar;
		return *this;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData>& Vec<Type, N, SwizzlingData>::operator*=(const Type& scalar)
	{
		for (uint8 i = 0; i < N; ++i)
			values[i] *= scalar;
		return *this;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData>& Vec<Type, N, SwizzlingData>::operator/=(const Type& scalar)
	{
		for (uint8 i = 0; i < N; ++i)
			values[i] /= scalar;
		return *this;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Type& Vec<Type, N, SwizzlingData>::operator[](uint32 index)
	{
		return At(index);
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Type Vec<Type, N, SwizzlingData>::operator[](uint32 index) const
	{
		return AtConst(index);
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Type& Vec<Type, N, SwizzlingData>::At(uint32 index)
	{
		RS_ASSERT_NO_MSG(index < N);
		return values[index];
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Type Vec<Type, N, SwizzlingData>::AtConst(uint32 index) const
	{
		RS_ASSERT_NO_MSG(index < N);
		return values[index];
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Type Vec<Type, N, SwizzlingData>::Dot(const Vec& other) const
	{
		Type result = static_cast<Type>(0);
		for (uint8 i = 0; i < N; ++i)
			result += values[i] * other.values[i];
		return result;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline void Vec<Type, N, SwizzlingData>::Copy(const Vec& other)
	{
		memcpy(&values[0], &other.values[0], sizeof(Type) * N);
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline void Vec<Type, N, SwizzlingData>::Move(const Vec&& other)
	{
		for (uint8 i = 0; i < N; ++i)
			values[i] = std::move(other.values[i]);
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline std::string Vec<Type, N, SwizzlingData>::ToString() const
	{
		std::string str;
		str += '[';
		for (uint8 i = 0; i < N; ++i)
		{
			str += Utils::Format("{}", values[i]);
			if (i != N - 1)
				str += ", ";
		}
		str += ']';
		return str;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData>::operator std::string() const
	{
		return ToString();
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	template<typename ...Types>
	inline void Vec<Type, N, SwizzlingData>::Create(uint32 n, Type type, Types ...types)
	{
		RS_ASSERT(n < N, "Too many arguments passed to the constructor! Vector expected at most {} arguments.", N);
		values[n] = type;
		Create(n+1, types...);
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline void Vec<Type, N, SwizzlingData>::Create(uint32 n)
	{
		if (n != 0 && n < N)
		{
			for (uint32 i = n; i < N; ++i)
				values[i] = values[n - 1];
		}
		RS_ASSERT_NO_MSG(n <= N);
	}

	// ------------ Global functions ------------
	template<typename Type>
	inline Vec<Type, 3> Cross(const Vec<Type, 3>& a, const Vec<Type, 3>& b)
	{
		Vec<Type, 3> result;
		result.x = a.y * b.z - a.z * b.y;
		result.y = a.z * b.x - a.x * b.z;
		result.z = a.x * b.y - a.y * b.x;
		return result;
	}

	using Vec2 = Vec<float, 2>;
	using Vec2u = Vec<uint32, 2>;
	using Vec2i = Vec<int32, 2>;

	using Vec3 = Vec<float, 3>;
	using Vec3u = Vec<uint32, 3>;
	using Vec3i = Vec<int32, 3>;

	using Vec4 = Vec<float, 4>;
	using Vec4u = Vec<uint32, 4>;
	using Vec4i = Vec<int32, 4>;
}