#pragma once

#include "Maths/RSVector.h"
#include "Utils/Misc/StringUtils.h" // Utils::Format

#ifndef RS_MATHS_COLUMN_MAJOR
#define RS_MATHS_COLUMN_MAJOR 1
#endif

namespace RS
{
	template<typename Type, uint32 Row, uint32 Col>
	struct Mat
	{
#if RS_MATHS_COLUMN_MAJOR
		RS::Vec<Type, Row> data[Col];
#else
		RS::Vec<Type, Col> data[Row];
#endif

#if RS_MATHS_COLUMN_MAJOR
		inline static const uint32 DATA_VECTOR_COUNT = Col;
		inline static const uint32 DATA_VECTOR_ELEMET_COUNT = Row;
#else
		inline static const uint32 DATA_VECTOR_COUNT = Row;
		inline static const uint32 DATA_VECTOR_ELEMET_COUNT = Col;
#endif

		Mat();
		Mat(const Type& value);
		// Initializes the matrix from row to row. Starting from the top left element.
		Mat(const std::initializer_list<Type>& list);

		Mat(const Mat& other);
		template<typename MType, uint32 Row2, uint32 Col2>
		Mat(const Mat<MType, Row2, Col2>& other);
		Mat(const Mat&& other);

		template<uint32 Row2, uint32 Col2>
		bool operator==(const Mat<Type, Row2, Col2>& other) const;
		template<uint32 Row2, uint32 Col2>
		bool operator!=(const Mat<Type, Row2, Col2>& other) const;

		bool operator==(const std::initializer_list<Type>& list) const;
		bool operator!=(const std::initializer_list<Type>& list) const;

		Mat& operator=(const Type& scalar);
		// Initializes the matrix from row to row. Starting from the top left element.
		Mat& operator=(const std::initializer_list<Type>& list);
		Mat& operator=(const Mat& other);
		//Mat& operator=(const Mat&& other);

		template<uint32 Row2, uint32 Col2>
		Mat<Type, Row, Col2> operator*(const Mat<Type, Row2, Col2>& other) const;
		Mat<Type, Row, Col>& operator*=(const Mat<Type, Row, Col>& other);

		Mat operator+(const Mat& other) const;
		Mat& operator+=(const Mat& other);
		Mat operator+(const Type& scalar) const;
		Mat& operator+=(const Type& scalar);
		Mat operator*(const Type& scalar) const;
		Mat& operator*=(const Type& scalar);
		Mat operator-(const Type& scalar) const;
		Mat& operator-=(const Type& scalar);
		Mat operator/(const Type& scalar) const;
		Mat& operator/=(const Type& scalar);

		// Note: These rely on the data order! Use with caution!
#if RS_MATHS_COLUMN_MAJOR
		Vec<Type, Row>& operator[](uint32 index);
		const Vec<Type, Row>& operator[](uint32 index) const;
#else
		Vec<Type, Col>& operator[](uint32 index);
		const Vec<Type, Col>& operator[](uint32 index) const;
#endif
		
		Type& At(uint32 row, uint32 col);
		Type AtConst(uint32 row, uint32 col) const;

		RS::Vec<Type, Col> GetRow(uint32 index) const;
		RS::Vec<Type, Row> GetCol(uint32 index) const;

		template<typename MType>
		void Copy(const Mat<MType, Row, Col>& other);
		void Move(const Mat&& other);

		Mat Identity() const;

		static const Mat ZERO;
		static const Mat IDENTITY;

		std::string ToString(bool oneLine = false, bool supportExtendedAscii = true) const;
		operator std::string() const;
	};

	template<typename Type, uint32 Row, uint32 Col>
	const Mat<Type, Row, Col> Mat<Type, Row, Col>::ZERO = Mat<Type, Row, Col>(static_cast<Type>(0));


	template<typename Type, uint32 Row, uint32 Col>
	const Mat<Type, Row, Col> Mat<Type, Row, Col>::IDENTITY = Mat<Type, Row, Col>().Identity();

	template<typename Type, uint32 Row, uint32 Col>
	inline Mat<Type, Row, Col>::Mat()
	{
		// Uses the default constructors for the data vectors.
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline Mat<Type, Row, Col>::Mat(const Type& value)
	{
		for (uint32 vecIndex = 0; DATA_VECTOR_COUNT; ++vecIndex)
			data[vecIndex] = value;
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline Mat<Type, Row, Col>::Mat(const std::initializer_list<Type>& list)
	{
		RS_ASSERT(list.size() == Row * Col, "Size does not match the size of the matrix!");

		uint32 index = 0;
		for (auto it = list.begin(); it != list.end(); it++)
		{
			uint32 col = index % Col;
			uint32 row = index / Col;
			At(row, col) = *it;
			index++;
		}
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline Mat<Type, Row, Col>::Mat(const Mat<Type, Row, Col>& other)
	{
		Copy(other);
	}

	template<typename Type, uint32 Row, uint32 Col>
	template<typename MType, uint32 Row2, uint32 Col2>
	inline Mat<Type, Row, Col>::Mat(const Mat<MType, Row2, Col2>& other)
	{
		if (Row == Row2 && Col == Col2)
			Copy<MType>(other);
		else
		{
			uint32 rows = std::min(Row, Row2);
			uint32 cols = std::min(Col, Col2);
			for (uint32 row = 0; row < rows; ++row)
			{
				for (uint32 col = 0; col < cols; ++col)
				{
					At(row, col) = static_cast<Type>(other.AtConst(row, col));
				}
			}
		}
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline Mat<Type, Row, Col>::Mat(const Mat&& other)
	{
		Move(std::move(other));
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline bool Mat<Type, Row, Col>::operator==(const std::initializer_list<Type>& list) const
	{
		RS_ASSERT(list.size() % 3 == 0 || list.size() % 2 == 0, "List need to have the same amount of elements as a matrix of at least two rows and columns!");
		
		if (Row * Col != list.size())
			return false;

		uint32 index = 0;
		for (auto it = list.begin(); it != list.end(); it++)
		{
			uint32 col = index % Col;
			uint32 row = index / Col;
			if (At(row, col) != *it)
				return false;
			index++;
		}

		return true;
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline bool Mat<Type, Row, Col>::operator!=(const std::initializer_list<Type>& list) const
	{
		return !(*this == list);
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline Mat<Type, Row, Col>& Mat<Type, Row, Col>::operator=(const Type& scalar)
	{
		for (uint32 vecIndex = 0; DATA_VECTOR_COUNT; ++vecIndex)
			data[vecIndex] = scalar;
		return *this;
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline Mat<Type, Row, Col>& Mat<Type, Row, Col>::operator=(const std::initializer_list<Type>& list)
	{
		RS_ASSERT((Row * Col) == list.size(), "List need to have the same amount of elements as the matrix! Want: {}, Got: {}", Row * Col, list.size());

		uint32 index = 0;
		for (auto it = list.begin(); it != list.end(); it++)
		{
			uint32 col = index % Col;
			uint32 row = index / Col;
			At(row, col) = *it;
			index++;
		}

		return *this;
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline Mat<Type, Row, Col>& Mat<Type, Row, Col>::operator=(const Mat& other)
	{
		Copy(other);
		return *this;
	}

	//template<typename Type, uint32 Row, uint32 Col>
	//inline Mat<Type, Row, Col>& Mat<Type, Row, Col>::operator=(const Mat&& other)
	//{
	//	Move(other);
	//	return *this;
	//}

	template<typename Type, uint32 Row, uint32 Col>
	inline Mat<Type, Row, Col> Mat<Type, Row, Col>::operator+(const Mat& other) const
	{
		Mat result(*this);
		result += other;
		return result;
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline Mat<Type, Row, Col>& Mat<Type, Row, Col>::operator+=(const Mat& other)
	{
		for (uint32 index = 0; index < Col; ++index)
		{
			uint32 col = index % Col;
			uint32 row = index / Col;
			At(row, col) += other.At(row, col);
		}
		return *this;
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline Mat<Type, Row, Col> Mat<Type, Row, Col>::operator+(const Type& scalar) const
	{
		Mat result(*this);
		result += scalar;
		return result;
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline Mat<Type, Row, Col>& Mat<Type, Row, Col>::operator+=(const Type& scalar)
	{
		for (uint32 vecIndex = 0; vecIndex < DATA_VECTOR_COUNT; ++vecIndex)
			data[vecIndex] += scalar;
		return *this;
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline Mat<Type, Row, Col> Mat<Type, Row, Col>::operator*(const Type& scalar) const
	{
		Mat result(*this);
		result *= scalar;
		return result;
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline Mat<Type, Row, Col>& Mat<Type, Row, Col>::operator*=(const Type& scalar)
	{
		for (uint32 vecIndex = 0; vecIndex < DATA_VECTOR_COUNT; ++vecIndex)
			data[vecIndex] *= scalar;
		return *this;
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline Mat<Type, Row, Col> Mat<Type, Row, Col>::operator-(const Type& scalar) const
	{
		Mat result(*this);
		result -= scalar;
		return result;
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline Mat<Type, Row, Col>& Mat<Type, Row, Col>::operator-=(const Type& scalar)
	{
		for (uint32 vecIndex = 0; vecIndex < DATA_VECTOR_COUNT; ++vecIndex)
			data[vecIndex] -= scalar;
		return *this;
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline Mat<Type, Row, Col> Mat<Type, Row, Col>::operator/(const Type& scalar) const
	{
		Mat result(*this);
		result /= scalar;
		return result;
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline Mat<Type, Row, Col>& Mat<Type, Row, Col>::operator/=(const Type& scalar)
	{
		for (uint32 vecIndex = 0; vecIndex < DATA_VECTOR_COUNT; ++vecIndex)
			data[vecIndex] /= scalar;
		return *this;
	}

	template<typename Type, uint32 Row, uint32 Col>
#if RS_MATHS_COLUMN_MAJOR
	inline Vec<Type, Row>& Mat<Type, Row, Col>::operator[](uint32 index)
#else
	inline Vec<Type, Col>& Mat<Type, Row, Col>::operator[](uint32 index)
#endif
	{
		RS_ASSERT(index < DATA_VECTOR_ELEMET_COUNT);
		return data[index];
	}

	template<typename Type, uint32 Row, uint32 Col>
#if RS_MATHS_COLUMN_MAJOR
	inline const Vec<Type, Row>& Mat<Type, Row, Col>::operator[](uint32 index) const
#else
	inline const Vec<Type, Col>& Mat<Type, Row, Col>::operator[](uint32 index) const
#endif
	{
		RS_ASSERT(index < DATA_VECTOR_ELEMET_COUNT);
		return data[index];
	}

	template<typename Type, uint32 Row, uint32 Col>
	template<uint32 Row2, uint32 Col2>
	inline bool Mat<Type, Row, Col>::operator==(const Mat<Type, Row2, Col2>& other) const
	{
		if (Row != Row2 || Col != Col2)
			return false;
		for (uint32 vecIndex = 0; vecIndex < DATA_VECTOR_COUNT; ++vecIndex)
		{
			if (data[vecIndex] != other.data[vecIndex])
				return false;
		}
		return true;
	}

	template<typename Type, uint32 Row, uint32 Col>
	template<uint32 Row2, uint32 Col2>
	inline bool Mat<Type, Row, Col>::operator!=(const Mat<Type, Row2, Col2>& other) const
	{
		return !(*this == other);
	}

	template<typename Type, uint32 Row, uint32 Col>
	template<uint32 Row2, uint32 Col2>
	inline Mat<Type, Row, Col2> Mat<Type, Row, Col>::operator*(const Mat<Type, Row2, Col2>& other) const
	{
		RS_ASSERT(Col == Row2, "Number of columns of the left matrix need to match the number of rows on the right!");

		Mat<Type, Row, Col2> result;
		for (uint32 row = 0; row < Row; ++row)
		{
			for (uint32 col = 0; col < Col2; ++col)
			{
				Type dot = static_cast<Type>(0);
				//dot = GetRow(row).Dot(GetCol(col));
				for (uint32 index = 0; index < Col; ++index)
					dot += AtConst(row, index) * other.AtConst(index, col);
				result.At(row, col) = dot;
			}
		}
		return result;
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline Mat<Type, Row, Col>& Mat<Type, Row, Col>::operator*=(const Mat<Type, Row, Col>& other)
	{
		Copy(*this * other);
		return *this;
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline Type& Mat<Type, Row, Col>::At(uint32 row, uint32 col)
	{
#if RS_MATHS_COLUMN_MAJOR
		return data[col][row];
#else
		return data[row][col];
#endif
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline Type Mat<Type, Row, Col>::AtConst(uint32 row, uint32 col) const
	{
#if RS_MATHS_COLUMN_MAJOR
		return data[col].AtConst(row);
#else
		return data[row].AtConst(col);
#endif
	}


	template<typename Type, uint32 Row, uint32 Col>
	inline RS::Vec<Type, Col> Mat<Type, Row, Col>::GetRow(uint32 index) const
	{
#if RS_MATHS_COLUMN_MAJOR
		RS::Vec<Type, Col> result;
		for (uint32 colIndex = 0; colIndex < Col; ++colIndex)
			result[colIndex] = AtConst(index, colIndex);
		return result;
#else
		return data[index];
#endif
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline RS::Vec<Type, Row> Mat<Type, Row, Col>::GetCol(uint32 index) const
	{

#if RS_MATHS_COLUMN_MAJOR
		return data[index];
#else
		RS::Vec<Type, Row> result;
		for (uint32 rowIndex = 0; rowIndex < Row; ++rowIndex)
			result[rowIndex] = AtConst(rowIndex, index);
		return result;
#endif
	}

	template<typename Type, uint32 Row, uint32 Col>
	template<typename MType>
	inline void Mat<Type, Row, Col>::Copy(const Mat<MType, Row, Col>& other)
	{
		for (uint32 i = 0; i < DATA_VECTOR_COUNT; ++i)
			data[i].Copy(other.data[i]);
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline void Mat<Type, Row, Col>::Move(const Mat&& other)
	{
		for (uint32 i = 0; i < DATA_VECTOR_COUNT; ++i)
			data[i].Move(std::move(other.data[i]));
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline Mat<Type, Row, Col> Mat<Type, Row, Col>::Identity() const
	{
		Mat result; // Will set it to zero.
		for (uint32 diag = 0; diag < std::min(Row, Col); ++diag)
			result.At(diag, diag) = static_cast<Type>(1);
		return result;
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline std::string Mat<Type, Row, Col>::ToString(bool oneLine, bool supportExtendedAscii) const
	{
		std::string str;
		for (int32 row = -1; row < (int32)(Row + 1); ++row)
		{
			if ((row == -1 || row == Row))
			{
				if (!oneLine && supportExtendedAscii)
				{
					uint8 a = 218u;
					uint8 b = 191u;
					if (row == Row)
					{
						a = 192u;
						b = 217u;
					}
					str += std::format("{}{: >{}}{}", (char)a, "", Col * 3, (char)b);
					str += '\n';
				}
				continue;
			}

			for (int32 col = -1; col < (int32)(Col + 1); ++col)
			{
				Type value = AtConst((row == -1 || row == Row) ? 0 : row, (col == -1 || col == Col) ? 0 : col);
				Type valueAbs = static_cast<Type>(std::abs(static_cast<double>(value)));
				std::string valueStr = std::format("{}", valueAbs);
				char sign = value < 0 ? '-' : ' ';
				valueStr = sign + valueStr;

				if (oneLine)
				{
					if (col == -1 && (row == 0))
						str += '[';
					else if (col == Col && (row == Row-1))
						str += ']';
					else if (col != -1 && col != Col)
					{
						str += valueStr;
						str += ' ';
					}
				}
				else
				{
					if (col == -1)
						str += '|';
					else if (col == Col)
						str += '|';
					else
					{
						str += valueStr;
						str += ' ';
					}
				}
			}

			if (oneLine && row != Row - 1)
				str += "; ";
			else if (!oneLine && row != Row)
				str += '\n';
		}
		return str;
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline Mat<Type, Row, Col>::operator std::string() const
	{
		return ToString(true);
	}

	// ------------ Global functions ------------
	template<typename Type, uint32 Row, uint32 Col>
	inline Mat<Type, Col, Row> Transpose(const Mat<Type, Row, Col>& mat)
	{
		Mat<Type, Col, Row> result;
		for (uint32 vecIndex = 0; vecIndex < Mat<Type, Row, Col>::DATA_VECTOR_COUNT; ++vecIndex)
		{
			for (uint32 elemIndex = 0; elemIndex < Mat<Type, Row, Col>::DATA_VECTOR_ELEMET_COUNT; ++elemIndex)
			{
				result.data[elemIndex][vecIndex] = mat.data[vecIndex][elemIndex];
			}
		}
		return result;
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline Type Determinant(const Mat<Type, Row, Col>& mat)
	{
		RS_ASSERT(false, "Determinant is not implemented for a {}x{} matrix!", Row, Col);
		return static_cast<Type>(0);
	}

	template<typename Type>
	inline Type Determinant(const Mat<Type, 2, 2>& mat)
	{
		return mat.AtConst(0, 0) * mat.AtConst(1, 1) - mat.AtConst(0, 1) * mat.AtConst(1, 0);
	}

	template<typename Type>
	inline Type Determinant(const Mat<Type, 3, 3>& mat)
	{
		const Type a = mat.AtConst(0, 0);
		const Type b = mat.AtConst(0, 1);
		const Type c = mat.AtConst(0, 2);
		const Type d = mat.AtConst(1, 0);
		const Type e = mat.AtConst(1, 1);
		const Type f = mat.AtConst(1, 2);
		const Type g = mat.AtConst(2, 0);
		const Type h = mat.AtConst(2, 1);
		const Type i = mat.AtConst(2, 2);
		return a*e*i + b*f*g + c*d*h - c*e*g - b*d*i - a*f*h;
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline Mat<Type, Row-1, Col-1> SubMatrix(const Mat<Type, Row, Col>& mat, uint32 i, uint32 j)
	{
		Mat<Type, Row-1, Col-1> newMat;
		for (uint32 row = 0, newRow = 0; row < Row; ++row)
		{
			for (uint32 col = 0, newCol = 0; col < Col; ++col)
			{
				if (row != i && col != j)
				{
					newMat.At(newRow, newCol) = mat.AtConst(row, col);
				}
				if (col != j) newCol++;
			}
			if (row != i) newRow++;
		}
		return newMat;
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline Type Minor(const Mat<Type, Row, Col>& mat, uint32 i, uint32 j)
	{
		Mat<Type, Row-1, Col-1> newMat = SubMatrix<Type, Row, Col>(mat, i, j);
		return Determinant<Type>(newMat);
	}


	template<typename Type, uint32 Row, uint32 Col>
	inline Type Cofactor(const Mat<Type, Row, Col>& mat, uint32 i, uint32 j)
	{
		return (((i + j) % 2) == 0 ? static_cast<Type>(1) : static_cast<Type>(-1)) * Minor(mat, i, j);
	}

	template<typename Type, uint32 T>
	inline Type Determinant(const Mat<Type, T, T>& mat)
	{
		Type result = static_cast<Type>(0);

		// Laplace expansion on the 0th column.
		uint32 j = 0;
		for (uint32 i = 0; i < 4; ++i)
			result += mat.AtConst(i, j) * Cofactor(mat, i, j);

		return result;
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline Mat<Type, Row, Col> Adjugate(const Mat<Type, Row, Col>& mat)
	{
		Mat<Type, Row, Col> comatrix;

		for (uint32 row = 0; row < Row; ++row)
		{
			for (uint32 col = 0; col < Col; ++col)
			{
				comatrix.At(row, col) = Cofactor(mat, row, col);
			}
		}

		Mat<Type, Col, Row> result = Transpose(comatrix);
		return result;
	}

	template<typename Type>
	inline Mat<Type, 2, 2> Inverse(const Mat<Type, 2, 2>& mat)
	{
		const Type a = mat.AtConst(0, 0);
		const Type b = mat.AtConst(0, 1);
		const Type c = mat.AtConst(1, 0);
		const Type d = mat.AtConst(1, 1);
		Type det = a*d - b*c;
		RS_ASSERT(std::abs(det) > static_cast<Type>(FLT_EPSILON), "Cannot take inverse of this matix! Determinant is not allowed to be zero, it was {}.", det);
		Mat<Type, 2, 2> adj =
		{
			d, -b,
			-c, a
		};
		return adj * (static_cast<Type>(1) / det);
	}

	template<typename Type, uint32 Row, uint32 Col>
	inline Mat<Type, Row, Col> Inverse(const Mat<Type, Row, Col>& mat)
	{
		Type det = Determinant(mat);
		RS_ASSERT(std::abs(det) > static_cast<Type>(FLT_EPSILON), "Cannot take inverse of this matix! Determinant is not allowed to be zero, it was {}.", det);
		Mat<Type, Row, Col> adj = Adjugate(mat);
		return adj * (static_cast<Type>(1) / det);
	}

	template<typename Type, uint32 Row, uint32 Col>
	std::ostream& operator<<(std::ostream& os, const Mat<Type, Col, Row>& value)
	{
		os << value.ToString(false);
		return os;
	}

	inline Mat<float, 4, 4> CreatePerspectiveProjectionMat4(float fov, float aspectRatio, float zFar, float zNear)
	{
		// LH to clip space. NDC space is in [-1, 1] for x and y, and [0, 1] for z.
		const float tanHalfFOV = (float)std::tan(fov * 0.5 * 3.1415 / 180);
		const float zRange = zFar - zNear;
		Mat<float, 4, 4> result =
		{
			1.0f / (tanHalfFOV * aspectRatio)	, 0.0f				, 0.0f				, 0.0f,
			0.0f								, 1.0f / tanHalfFOV	, 0.0f				, 0.0f,
			0.0f								, 0.0f				, zFar / zRange		, -(zFar * zNear) / zRange,
			0.0f								, 0.0f				, 1.0f				, 0.0f
		};
		return result;
	}

	inline Mat<float, 4, 4> CreatePerspectiveProjectionMat4RH(float fov, float aspectRatio, float zFar, float zNear)
	{
		// RH to clip space. NDC space is in [-1, 1] for x and y, and [0, 1] for z.
		const float tanHalfFOV = (float)std::tan(fov * 0.5 * 3.1415 / 180);
		const float zRange = zFar - zNear;
		Mat<float, 4, 4> result =
		{
			1.0f / (tanHalfFOV * aspectRatio)	, 0.0f				, 0.0f				, 0.0f,
			0.0f								, 1.0f / tanHalfFOV	, 0.0f				, 0.0f,
			0.0f								, 0.0f				, zFar / zRange		, (zFar * zNear) / zRange,
			0.0f								, 0.0f				, -1.0f				, 0.0f
		};
		return result;
	}

	inline Mat<float, 4, 4> CreateOrthographicProjectionMat4(float left, float right, float top, float bottom, float zNear, float zFar)
	{
		// LH to clip space [0, 1]
		const float zRange = zFar - zNear;
		Mat<float, 4, 4> result =
		{
			2.0f / (right - left)	, 0.0f					, 0.0f				, 0.f,
			0.0f					, 2.0f / (top - bottom)	, 0.0f				, 0.f,
			0.0f					, 0.0f					, -1.0f / zRange	, -zNear / zRange,
			0.0f					, 0.0f					, 0.0f				, 1.0f
		};
		return result;
	}

	inline Mat<float, 4, 4> CreateCameraMat4(Mat<float, 4, 4>& mat, const Vec<float, 3>& right, const Vec<float, 3>& up, const Vec<float, 3>& forward, const Vec<float, 3>& position)
	{
		mat.At(0, 0) = right.x;		mat.At(0, 1) = right.y;		mat.At(0, 2) = right.z;		mat.At(0, 3) = -position.Dot(right);
		mat.At(1, 0) = up.x;		mat.At(1, 1) = up.y;		mat.At(1, 2) = up.z;		mat.At(1, 3) = -position.Dot(up);
		mat.At(2, 0) = forward.x;	mat.At(2, 1) = forward.y;	mat.At(2, 2) = forward.z;	mat.At(2, 3) = -position.Dot(forward);
		mat.At(3, 0) = 0;			mat.At(3, 1) = 0;			mat.At(3, 2) = 0;			mat.At(3, 3) = 1.0f;

		return mat;
	}

	inline Mat<float, 4, 4> CreateCameraMat4RH(Mat<float, 4, 4>& mat, const Vec<float, 3>& right, const Vec<float, 3>& up, const Vec<float, 3>& forward, const Vec<float, 3>& position)
	{
		// RH
		mat.At(0, 0) = right.x;		mat.At(0, 1) = right.y;		mat.At(0, 2) = right.z;	mat.At(0, 3) = position.Dot(right);
		mat.At(1, 0) = up.x;		mat.At(1, 1) = up.y;		mat.At(1, 2) = up.z;		mat.At(1, 3) = position.Dot(up);
		mat.At(2, 0) = -forward.x;	mat.At(2, 1) = -forward.y;	mat.At(2, 2) = -forward.z;	mat.At(2, 3) = position.Dot(-forward);
		mat.At(3, 0) = 0;			mat.At(3, 1) = 0;			mat.At(3, 2) = 0;			mat.At(3, 3) = 1.0f;

		return mat;
	}

	inline Mat<float, 4, 4> CreateCameraMat4(const Vec<float, 3>& direction, const Vec<float, 3>& worldUp, const Vec<float, 3>& position)
	{
		// LH
		Mat<float, 4, 4> result;
		Vec<float, 3> right = worldUp;
		right = RS::Normalize(RS::Cross(direction, right));
		Vec<float, 3> up = RS::Normalize(RS::Cross(right, direction));
		return CreateCameraMat4(result, right, up, direction, position);
	}

	inline Mat<float, 4, 4> CreateCameraMat4RH(const Vec<float, 3>& direction, const Vec<float, 3>& worldUp, const Vec<float, 3>& position)
	{
		// RH
		Mat<float, 4, 4> result;
		Vec<float, 3> right = worldUp;
		right = RS::Normalize(RS::Cross(direction, right));
		Vec<float, 3> up = RS::Normalize(RS::Cross(right, direction));
		return CreateCameraMat4RH(result, right, up, direction, position);
	}

	using Mat2 = Mat<float, 2, 2>;
	using Mat2u = Mat<uint32, 2, 2>;
	using Mat2i = Mat<int32, 2, 2>;

	using Mat3 = Mat<float, 3, 3>;
	using Mat3u = Mat<uint32, 3, 3>;
	using Mat3i = Mat<int32, 3, 3>;

	using Mat32 = Mat<float, 3, 2>;
	using Mat32u = Mat<uint32, 3, 2>;
	using Mat32i = Mat<int32, 3, 2>;

	using Mat23 = Mat<float, 2, 3>;
	using Mat23u = Mat<uint32, 2, 3>;
	using Mat23i = Mat<int32, 2, 3>;

	using Mat4 = Mat<float, 4, 4>;
	using Mat4u = Mat<uint32, 4, 4>;
	using Mat4i = Mat<int32, 4, 4>;

	using Mat42 = Mat<float, 4, 2>;
	using Mat42u = Mat<uint32, 4, 2>;
	using Mat42i = Mat<int32, 4, 2>;

	using Mat43 = Mat<float, 4, 3>;
	using Mat43u = Mat<uint32, 4, 3>;
	using Mat43i = Mat<int32, 4, 3>;

	using Mat24 = Mat<float, 2, 4>;
	using Mat24u = Mat<uint32, 2, 4>;
	using Mat24i = Mat<int32, 2, 4>;

	using Mat34 = Mat<float, 3, 4>;
	using Mat34u = Mat<uint32, 3, 4>;
	using Mat34i = Mat<int32, 3, 4>;
}
