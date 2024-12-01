#pragma once

// TODO: Remove as much inheritance as possible!
//		It was made like this such that it would be easier to implement, but it is slower in practice because of the amount of v-tables.

#include "RSVectorDataSwizzling.h"
#include "Utils/Misc/StringUtils.h" // Utils::Format
#include "Maths/Maths.h"

#define RS_CONVERT_BETWEEN_GLM 1
#if RS_CONVERT_BETWEEN_GLM
#include "Maths/GLMDefines.h"
#include <glm/detail/type_vec3.hpp>
#endif

#include <concepts>

// TODO: Change this to RS when done!
namespace RS_New
{
	struct NoInitStruct { };
	inline static const NoInitStruct NoInit{};

	struct Empty {};

	template<NumberType Type, auto N>
	struct DataContainer {
		Type data[N];
	};
	template<NumberType Type>
	struct DataContainer<Type, 2u>
	{
		union
		{
			Type data[2];
			struct
			{
				Type x;
				Type y;
			};
		};
	};
	template<NumberType Type>
	struct DataContainer<Type, 3u>
	{
		union
		{
			Type data[3];
			struct
			{
				Type x;
				Type y;
				Type z;
			};
		};
	};
	template<NumberType Type>
	struct DataContainer<Type, 4u>
	{
		union
		{
			Type data[4];
			struct
			{
				Type x;
				Type y;
				Type z;
				Type w;
			};
		};
	};

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	class VecNew final : public DataContainer<Type, N>
	{
	public:
		using SizeType = decltype(N);
		//Type data[N];

	public:
		inline static const SizeType Size = N;
		static const VecNew<N, Type> Ones;
		static const VecNew<N, Type> Zeros;

	public:
		VecNew(); // Defaults the values to zero.
		VecNew(Type value); // Sets very element to the specified value.
		VecNew(NoInitStruct); // Example: Vec<2, float>(NoInit);

		// Example: Vec<2, float>({1.0f, 2.0f});
		VecNew(const std::initializer_list<Type>& list);
		VecNew(const VecNew& other);
		VecNew(const VecNew&& other);

		// Example: Vec<2, float>(aVecNew3i);
		template<auto M, NumberType Type2> requires VecSizeConstant<decltype(M)>
		VecNew(const VecNew<M, Type2>& other);

		// Example: Vec<2, float>(1.0f, 3.0f);
		template<NumberType... Types>
		VecNew(Type type, Types... types);

		// Example: Vec<2, float>(anArrayOfTwoFloats, 2);
		VecNew(SizeType size, const Type* pData);

		VecNew& operator=(const VecNew& other);
		VecNew& operator=(const VecNew&& other);
		template<auto M, NumberType Type2> requires VecSizeConstant<decltype(M)>
		VecNew& operator=(const VecNew<M, Type2>& other);
		VecNew& operator=(const Type& scalar);

		Type& At(SizeType index);
		const Type& At(SizeType index) const;
		Type& operator[](SizeType index);
		const Type& operator[](SizeType index) const;

		template<auto M, NumberType Type2> requires VecSizeConstant<decltype(M)>
		bool operator==(const VecNew<M, Type2>& other) const;
		template<auto M, NumberType Type2> requires VecSizeConstant<decltype(M)>
		bool operator!=(const VecNew<M, Type2>& other) const;

		bool operator==(const std::initializer_list<Type>& list) const;
		bool operator!=(const std::initializer_list<Type>& list) const;

		VecNew operator-() const;

		VecNew operator+(const VecNew& other) const;
		VecNew operator-(const VecNew& other) const;
		VecNew operator*(const VecNew& other) const;
		VecNew operator/(const VecNew& other) const;
		VecNew operator+(const Type& scalar) const;
		VecNew operator-(const Type& scalar) const;
		VecNew operator*(const Type& scalar) const;
		VecNew operator/(const Type& scalar) const;

		VecNew& operator+=(const VecNew& other);
		VecNew& operator-=(const VecNew& other);
		VecNew& operator*=(const VecNew& other);
		VecNew& operator/=(const VecNew& other);
		VecNew& operator+=(const Type& scalar);
		VecNew& operator-=(const Type& scalar);
		VecNew& operator*=(const Type& scalar);
		VecNew& operator/=(const Type& scalar);

		// TODO: All generic vec functions can be put into another file.
		
		Type Length2() const;
		template<std::floating_point FloatType>
		FloatType Length() const;

		template<NumberType Type2>
		Type Dot(const VecNew<N, Type2>& other) const;

		VecNew Normalize() const;

		void SetData(Type value);
		void SetData(Type value, SizeType size);
		void SetData(const Type* pData, SizeType size);

		void Copy(const VecNew& other);
		void Move(const VecNew&& other);
		template<auto M, NumberType Type2> requires VecSizeConstant<decltype(M)>
		void Copy(const VecNew<M, Type2>& other);

	private:
		template<NumberType... Types>
		void CreateV(SizeType n, Type type, Types... types);

		void CreateV(SizeType n);
	};

	// Deduction guid
	//template<VecSizeConstant T, typename U>
	//Vec(T, U) -> Vec<T, U>;

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline const VecNew<N, Type> VecNew<N, Type>::Ones = VecNew<N, Type>((Type)1);
	
	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline const VecNew<N, Type> VecNew<N, Type>::Zeros = VecNew<N, Type>((Type)0);

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type>::VecNew()
	{
		SetData((Type)0, N);
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type>::VecNew(Type value)
	{
		SetData(value, N);
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type>::VecNew(NoInitStruct) { }

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type>::VecNew(const std::initializer_list<Type>& list)
	{
		SizeType i = (SizeType)0;
		for (auto it = list.begin(); it != list.end(); it++, i++)
			this->data[i] = *it;
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type>::VecNew(const VecNew& other)
	{
		Copy(other);
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type>::VecNew(const VecNew&& other)
	{
		Move(std::move(other));
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	template<auto M, NumberType Type2> requires VecSizeConstant<decltype(M)>
	inline VecNew<N, Type>::VecNew(const VecNew<M, Type2>& other)
	{
		Copy(other);
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	template<NumberType ...Types>
	inline VecNew<N, Type>::VecNew(Type value, Types ...values)
	{
		CreateV((SizeType)0, value, values...);
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type>::VecNew(SizeType size, const Type* pData)
	{
		SetData(pData, size);
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type>& VecNew<N, Type>::operator=(const VecNew& other)
	{
		Copy(other);
		return *this;
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type>& VecNew<N, Type>::operator=(const VecNew&& other)
	{
		Move(std::move(other));
		return *this;
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	template<auto M, NumberType Type2> requires VecSizeConstant<decltype(M)>
	inline VecNew<N, Type>& VecNew<N, Type>::operator=(const VecNew<M, Type2>& other)
	{
		Copy(other);
		return *this;
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type>& VecNew<N, Type>::operator=(const Type& scalar)
	{
		SetData(scalar, N);
		return *this;
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline Type& VecNew<N, Type>::At(SizeType index)
	{
		return this->data[index];
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline const Type& VecNew<N, Type>::At(SizeType index) const
	{
		return this->data[index];
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline Type& VecNew<N, Type>::operator[](SizeType index)
	{
		return At(index);
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline const Type& VecNew<N, Type>::operator[](SizeType index) const
	{
		return At(index);
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	template<auto M, NumberType Type2> requires VecSizeConstant<decltype(M)>
	inline bool VecNew<N, Type>::operator==(const VecNew<M, Type2>& other) const
	{
		if (N != M) return false;
		for (SizeType i = (SizeType)0; i < N; ++i)
			if (RS::Maths::Abs((Type)(this->data[i] - (Type)other.data[i])) > static_cast<Type>(FLT_EPSILON)) return false;
		return true;
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	template<auto M, NumberType Type2> requires VecSizeConstant<decltype(M)>
	inline bool VecNew<N, Type>::operator!=(const VecNew<M, Type2>& other) const
	{
		return !(*this == other);
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline bool VecNew<N, Type>::operator==(const std::initializer_list<Type>& list) const
	{
		if ((SizeType)list.size() != N) return false;
		SizeType i = (SizeType)0;
		for (auto it = list.begin(); it != list.end(); it++, i++)
			if (RS::Maths::Abs((Type)(this->data[i] - *it)) > static_cast<Type>(FLT_EPSILON)) return false;
		return true;
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline bool VecNew<N, Type>::operator!=(const std::initializer_list<Type>& list) const
	{
		return !(*this == list);
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type> VecNew<N, Type>::operator-() const
	{
		VecNew<N, Type> result(*this);
		for (SizeType i = (SizeType)0; i < N; ++i)
			result.data[i] = -result.data[i];
		return result;
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type> VecNew<N, Type>::operator+(const VecNew& other) const
	{
		VecNew<N, Type> result(*this);
		result += other;
		return result;
	}
	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type> VecNew<N, Type>::operator-(const VecNew& other) const
	{
		VecNew<N, Type> result(*this);
		result -= other;
		return result;
	}
	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type> VecNew<N, Type>::operator*(const VecNew& other) const
	{
		VecNew<N, Type> result(*this);
		result *= other;
		return result;
	}
	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type> VecNew<N, Type>::operator/(const VecNew& other) const
	{
		VecNew<N, Type> result(*this);
		result /= other;
		return result;
	}
	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type> VecNew<N, Type>::operator+(const Type& scalar) const
	{
		VecNew<N, Type> result(*this);
		result += scalar;
		return result;
	}
	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type> VecNew<N, Type>::operator-(const Type& scalar) const
	{
		VecNew<N, Type> result(*this);
		result -= scalar;
		return result;
	}
	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type> VecNew<N, Type>::operator*(const Type& scalar) const
	{
		VecNew<N, Type> result(*this);
		result *= scalar;
		return result;
	}
	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type> VecNew<N, Type>::operator/(const Type& scalar) const
	{
		VecNew<N, Type> result(*this);
		result /= scalar;
		return result;
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type>& VecNew<N, Type>::operator+=(const VecNew& other)
	{
		for (SizeType i = (SizeType)0; i < N; ++i)
			this->data[i] += other.data[i];
		return *this;
	}
	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type>& VecNew<N, Type>::operator-=(const VecNew& other)
	{
		for (SizeType i = (SizeType)0; i < N; ++i)
			this->data[i] -= other.data[i];
		return *this;
	}
	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type>& VecNew<N, Type>::operator*=(const VecNew& other)
	{
		for (SizeType i = (SizeType)0; i < N; ++i)
			this->data[i] *= other.data[i];
		return *this;
	}
	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type>& VecNew<N, Type>::operator/=(const VecNew& other)
	{
		for (SizeType i = (SizeType)0; i < N; ++i)
			this->data[i] /= other.data[i];
		return *this;
	}
	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type>& VecNew<N, Type>::operator+=(const Type& scalar)
	{
		for (SizeType i = (SizeType)0; i < N; ++i)
			this->data[i] += scalar;
		return *this;
	}
	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type>& VecNew<N, Type>::operator-=(const Type& scalar)
	{
		for (SizeType i = (SizeType)0; i < N; ++i)
			this->data[i] -= scalar;
		return *this;
	}
	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type>& VecNew<N, Type>::operator*=(const Type& scalar)
	{
		for (SizeType i = (SizeType)0; i < N; ++i)
			this->data[i] *= scalar;
		return *this;
	}
	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type>& VecNew<N, Type>::operator/=(const Type& scalar)
	{
		for (SizeType i = (SizeType)0; i < N; ++i)
			this->data[i] /= scalar;
		return *this;
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline Type VecNew<N, Type>::Length2() const
	{
		Type result = (Type)0;
		for (SizeType i = (SizeType)0; i < N; ++i)
			result += (this->data[i] * this->data[i]);
		return result;
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	template<std::floating_point FloatType>
	inline FloatType VecNew<N, Type>::Length() const
	{
		return (FloatType)std::sqrt((FloatType)Length2());
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	template<NumberType Type2>
	inline Type VecNew<N, Type>::Dot(const VecNew<N, Type2>& other) const
	{
		Type result = (Type)0;
		for (SizeType i = (SizeType)0; i < N; ++i)
			result += (this->data[i] * other.data[i]);
		return result;
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline VecNew<N, Type> VecNew<N, Type>::Normalize() const
	{
		Type len = (Type)Length<double>();
		return *this / len;
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline void VecNew<N, Type>::SetData(Type value)
	{
		for (SizeType i = (SizeType)0; i < N; ++i)
			this->data[i] = value;
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline void VecNew<N, Type>::SetData(Type value, SizeType size)
	{
		const SizeType count = size < N ? size : N;
		for (SizeType i = (SizeType)0; i < count; ++i)
			this->data[i] = value;
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline void VecNew<N, Type>::SetData(const Type* pData, SizeType size)
	{
		RS_ASSERT(pData != nullptr, "pData need to be an array of entries!");
		SizeType count = size < N ? size : N;
		for (SizeType i = (SizeType)0; i < count; ++i)
			this->data[i] = pData[i];
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline void VecNew<N, Type>::Copy(const VecNew& other)
	{
		for (SizeType i = (SizeType)0; i < N; ++i)
			this->data[i] = other.data[i];
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline void VecNew<N, Type>::Move(const VecNew&& other)
	{
		for (SizeType i = (SizeType)0; i < N; ++i)
			this->data[i] = std::move(other.data[i]);
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	template<auto M, NumberType Type2> requires VecSizeConstant<decltype(M)>
	inline void VecNew<N, Type>::Copy(const VecNew<M, Type2>& other)
	{
		const SizeType count = M < N ? M : N;
		for (SizeType i = (SizeType)0; i < count; ++i)
			this->data[i] = other.data[i];
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	template<NumberType ...Types>
	inline void VecNew<N, Type>::CreateV(SizeType n, Type value, Types ...values)
	{
		RS_ASSERT(n < N, "Too many arguments passed to the constructor! Vector expected at most {} arguments.", N);
		this->data[n] = value;
		CreateV(n + 1, values...);
	}

	template<auto N, NumberType Type> requires VecSizeConstant<decltype(N)>
	inline void VecNew<N, Type>::CreateV(SizeType n)
	{
		if (n != 0 && n < N)
		{
			for (uint32 i = n; i < N; ++i)
				this->data[i] = this->data[n - 1];
		}
		RS_ASSERT_NO_MSG(n <= N);
	}

	using Vec2 = VecNew<2u, float>;
	using Vec2u = VecNew<2u, uint32>;
	using Vec2i = VecNew<2u, int32>;
	
	using Vec3 = VecNew<3u, float>;
	using Vec3u = VecNew<3u, uint32>;
	using Vec3i = VecNew<3u, int32>;

	using Vec4 = VecNew<4u, float>;
	using Vec4u = VecNew<4u, uint32>;
	using Vec4i = VecNew<4u, int32>;
}

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

		Vec(const Type* pData, uint32 size = UINT32_MAX);

#if RS_CONVERT_BETWEEN_GLM
		operator glm::vec<N, Type, glm::qualifier::highp>() const;
#endif

		template<uint32 M>
		bool operator==(const Vec<Type, M, VectorSwizzlingData<Type, M>>& other) const;
		template<uint32 M>
		bool operator!=(const Vec<Type, M, VectorSwizzlingData<Type, M>>& other) const;

		bool operator==(const std::initializer_list<Type>& list) const;
		bool operator!=(const std::initializer_list<Type>& list) const;

		Vec& operator=(const Type& scalar);
		Vec& operator=(const std::initializer_list<Type>& list);
		Vec& operator=(const Vec& other);
		//Vec& operator=(const Vec&& other);

		Vec operator-() const;

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
		Type Length() const;

		void Copy(const Vec& other);
		void Move(const Vec&& other);

		static const Vec<Type, N, SwizzlingData> ZERO;

		std::string ToString() const;
		operator std::string() const;

		Type* Data() const;
		void SetData(const Type* pData, uint32 size = UINT32_MAX);

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
	inline Vec<Type, N, SwizzlingData>::Vec(const Type* pData, uint32 size)
		: SwizzlingData(false)
	{
		SetData(pData, size);
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
			this->values[i] = static_cast<Type>(other.values[i]);

		if (M < N)
			Create(M, types...);
	}

#if RS_CONVERT_BETWEEN_GLM
	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData>::operator glm::vec<N, Type, glm::qualifier::highp>() const
	{
		RS_ASSERT(N > 1 && N < 5, "Conversion to glm::vec is only supported for size 2, 3 and 4.");
		if constexpr (N == 2)
		{
			return glm::vec<N, Type, glm::qualifier::highp>(this->values[0], this->values[1]);
		}
		else if constexpr (N == 3)
		{
			return glm::vec<N, Type, glm::qualifier::highp>(this->values[0], this->values[1], this->values[2]);
		}
		else if constexpr (N == 4)
		{
			return glm::vec<N, Type, glm::qualifier::highp>(this->values[0], this->values[1], this->values[2], this->values[4]);
		}
		return glm::vec<N, Type, glm::qualifier::highp>();
	}
#endif

	template<typename Type, uint32 N, typename SwizzlingData>
	template<uint32 M>
	inline bool Vec<Type, N, SwizzlingData>::operator==(const Vec<Type, M, VectorSwizzlingData<Type, M>>& other) const
	{
		if (N != M) return false;
		for (uint32 i = 0; i < N; ++i)
		{
			if (std::abs(this->values[i++] - other.values[i]) > static_cast<Type>(FLT_EPSILON))
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
		for (auto it = list.begin(); it != list.end(); it++)
		{
			if (std::abs(this->values[i++] - *it) > static_cast<Type>(FLT_EPSILON))
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
			this->values[i] = scalar;
		return *this;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData>& Vec<Type, N, SwizzlingData>::operator=(const std::initializer_list<Type>& list)
	{
		RS_ASSERT(list.size() > 0, "List must have at least one element!");
		uint32 i = 0;
		for (auto it = list.begin(); it != list.end() && i < N; it++)
			this->values[i++] = *it;
		for (uint32 j = i; j < N; ++j)
			this->values[j] = this->values[i - 1];
		return *this;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData>& Vec<Type, N, SwizzlingData>::operator=(const Vec& other)
	{
		Copy(other);
		return *this;
	}

	//template<typename Type, uint32 N, typename SwizzlingData>
	//inline Vec<Type, N, SwizzlingData>& Vec<Type, N, SwizzlingData>::operator=(const Vec&& other)
	//{
	//	Move(other);
	//	return *this;
	//}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData> Vec<Type, N, SwizzlingData>::operator-() const
	{
		Vec result(*this);
		for (uint8 i = 0; i < N; ++i)
			result.values[i] = -result.values[i];
		return result;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData> Vec<Type, N, SwizzlingData>::operator+(const Vec& other) const
	{
		Vec result(*this);
		result += other;
		return result;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData> Vec<Type, N, SwizzlingData>::operator-(const Vec& other) const
	{
		Vec result(*this);
		result -= other;
		return result;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData> Vec<Type, N, SwizzlingData>::operator+(const Type& scalar) const
	{
		Vec result(*this);
		result += scalar;
		return result;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData> Vec<Type, N, SwizzlingData>::operator-(const Type& scalar) const
	{
		Vec result(*this);
		result -= scalar;
		return result;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData> Vec<Type, N, SwizzlingData>::operator*(const Type& scalar) const
	{
		Vec result(*this);
		result *= scalar;
		return result;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData> Vec<Type, N, SwizzlingData>::operator/(const Type& scalar) const
	{
		Vec result(*this);
		result /= scalar;
		return result;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData>& Vec<Type, N, SwizzlingData>::operator+=(const Vec& other)
	{
		for (uint8 i = 0; i < N; ++i)
			this->values[i] += other.values[i];
		return *this;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData>& Vec<Type, N, SwizzlingData>::operator-=(const Vec& other)
	{
		for (uint8 i = 0; i < N; ++i)
			this->values[i] -= other.values[i];
		return *this;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData>& Vec<Type, N, SwizzlingData>::operator+=(const Type& scalar)
	{
		for (uint8 i = 0; i < N; ++i)
			this->values[i] += scalar;
		return *this;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData>& Vec<Type, N, SwizzlingData>::operator-=(const Type& scalar)
	{
		for (uint8 i = 0; i < N; ++i)
			this->values[i] -= scalar;
		return *this;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData>& Vec<Type, N, SwizzlingData>::operator*=(const Type& scalar)
	{
		for (uint8 i = 0; i < N; ++i)
			this->values[i] *= scalar;
		return *this;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Vec<Type, N, SwizzlingData>& Vec<Type, N, SwizzlingData>::operator/=(const Type& scalar)
	{
		for (uint8 i = 0; i < N; ++i)
			this->values[i] /= scalar;
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
		return this->values[index];
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Type Vec<Type, N, SwizzlingData>::AtConst(uint32 index) const
	{
		RS_ASSERT_NO_MSG(index < N);
		return this->values[index];
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Type Vec<Type, N, SwizzlingData>::Dot(const Vec& other) const
	{
		Type result = static_cast<Type>(0);
		for (uint8 i = 0; i < N; ++i)
			result += this->values[i] * other.values[i];
		return result;
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline Type Vec<Type, N, SwizzlingData>::Length() const
	{
		return (Type)std::sqrtf((float)Dot(*this));
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline void Vec<Type, N, SwizzlingData>::Copy(const Vec& other)
	{
		memcpy(&this->values[0], &other.values[0], sizeof(Type) * N);
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline void Vec<Type, N, SwizzlingData>::Move(const Vec&& other)
	{
		for (uint8 i = 0; i < N; ++i)
			this->values[i] = std::move(other.values[i]);
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline std::string Vec<Type, N, SwizzlingData>::ToString() const
	{
		std::string str;
		str += '[';
		for (uint8 i = 0; i < N; ++i)
		{
			str += std::format("{}", this->values[i]);
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
	inline Type* Vec<Type, N, SwizzlingData>::Data() const
	{
		RS_ASSERT_NO_MSG(N > 0);
		return &this->values[0];
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline void Vec<Type, N, SwizzlingData>::SetData(const Type* pData, uint32 size)
	{
		RS_ASSERT_NO_MSG(pData != nullptr);

		for (uint32 i = 0; i < std::min(N, size); ++i)
			this->values[i] = pData[i];
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	template<typename ...Types>
	inline void Vec<Type, N, SwizzlingData>::Create(uint32 n, Type type, Types ...types)
	{
		RS_ASSERT(n < N, "Too many arguments passed to the constructor! Vector expected at most {} arguments.", N);
		this->values[n] = type;
		Create(n+1, types...);
	}

	template<typename Type, uint32 N, typename SwizzlingData>
	inline void Vec<Type, N, SwizzlingData>::Create(uint32 n)
	{
		if (n != 0 && n < N)
		{
			for (uint32 i = n; i < N; ++i)
				this->values[i] = this->values[n - 1];
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

	template<typename Type, uint32 N>
	inline Vec<Type, N> Normalize(const Vec<Type, N>& v)
	{
		Type length = v.Length();
		Vec<Type, N> result(v);
		result /= length;
		return result;
	}

	template<typename Type, uint32 N>
	inline Vec<Type, N> Project(const Vec<Type, N>& left, const Vec<Type, N>& right)
	{
		Type dlr = left.Dot(right);
		Type drr = right.Dot(right);
		return right * (dlr / drr);
	}

	// Rotation around the axis following the right hand rule.
	template<typename Type>
	inline Vec<Type, 3> Rotate(const Vec<Type, 3>& v, float angle, const Vec<Type, 3>& axis)
	{
		Vec<Type, 3> vPAxis = Project<Type, 3>(v, axis);
		Vec<Type, 3> vOAxis = v - vPAxis;
		float x = (float)std::cos(angle);
		float y = (float)std::sin(angle);
		Vec<Type, 3> vOr = (Normalize<Type, 3>(vOAxis) * x + Normalize<Type, 3>(Cross<Type>(vOAxis, axis)) * y) * vOAxis.Length();
		return vOr + vPAxis;
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