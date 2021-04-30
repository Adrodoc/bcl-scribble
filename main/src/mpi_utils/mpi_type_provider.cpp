#pragma once

#include <mpi.h>

template <typename T>
struct mpi_type_provider;

template <>
struct mpi_type_provider<float>
{
        static constexpr MPI_Datatype mpi_type = MPI_FLOAT;
};

template <>
struct mpi_type_provider<double>
{
        static constexpr MPI_Datatype mpi_type = MPI_DOUBLE;
};

template <>
struct mpi_type_provider<long double>
{
        static constexpr MPI_Datatype mpi_type = MPI_LONG_DOUBLE;
};

template <>
struct mpi_type_provider<bool>
{
        static constexpr MPI_Datatype mpi_type = MPI_CXX_BOOL;
};

template <>
struct mpi_type_provider<int8_t>
{
        static constexpr MPI_Datatype mpi_type = MPI_INT8_T;
};

template <>
struct mpi_type_provider<int16_t>
{
        static constexpr MPI_Datatype mpi_type = MPI_INT16_T;
};

template <>
struct mpi_type_provider<int32_t>
{
        static constexpr MPI_Datatype mpi_type = MPI_INT32_T;
};

template <>
struct mpi_type_provider<int64_t>
{
        static constexpr MPI_Datatype mpi_type = MPI_INT64_T;
};

template <>
struct mpi_type_provider<uint8_t>
{
        static constexpr MPI_Datatype mpi_type = MPI_UINT8_T;
};

template <>
struct mpi_type_provider<uint16_t>
{
        static constexpr MPI_Datatype mpi_type = MPI_UINT16_T;
};

template <>
struct mpi_type_provider<uint32_t>
{
        static constexpr MPI_Datatype mpi_type = MPI_UINT32_T;
};

template <>
struct mpi_type_provider<uint64_t>
{
        static constexpr MPI_Datatype mpi_type = MPI_UINT64_T;
};

template <typename T>
MPI_Datatype get_mpi_type()
{
        return mpi_type_provider<T>::mpi_type;
}
