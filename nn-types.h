#pragma once

#include <vector>

#include <stddef.h> // for size_t

#include <vector>

// various data types involved in NN model data

typedef std::vector<unsigned> TensorShape;

enum WidthHeight {
	WIDTH,
	HEIGHT
};

size_t tensorFlatSize(const TensorShape &shape);
unsigned tensorNumMultiDims(const TensorShape &shape);
TensorShape tensorGetLastDims(const TensorShape &shape, unsigned ndims);
