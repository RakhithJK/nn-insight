

#include "plugin-interface.h"

#include <cctype>

std::ostream& operator<<(std::ostream &os, PluginInterface::OperatorKind okind) {
#define CASE(kind) case PluginInterface::Kind##kind: os << #kind; break;
	switch (okind) {
  	CASE(Conv2D) CASE(DepthwiseConv2D) CASE(Pad) CASE(FullyConnected) CASE(MaxPool) CASE(AveragePool) CASE(Add) CASE(Relu) CASE(Relu6) CASE(LeakyRelu)
	CASE(Tanh) CASE(Sub) CASE(Mul) CASE(Div) CASE(Maximum) CASE(Minimum) CASE(Transpose) CASE(Reshape) CASE(Softmax) CASE(Concatenation)
	CASE(StridedSlice) CASE(Mean)
	CASE(Unknown)
	}
#undef CASE
	return os;
}

std::ostream& operator<<(std::ostream &os, PluginInterface::OperatorOptionName optName) {
	std::string s;
#define CASE(name) case PluginInterface::OperatorOption_##name: s = #name; break;
	switch (optName) {
		CASE(UNKNOWN)
		CASE(ALIGN_CORNERS)
		CASE(ALPHA)
		CASE(AXIS)
		CASE(BATCH_DIM)
		CASE(BEGIN_MASK)
		CASE(BETA)
		CASE(BIAS)
		CASE(BLOCK_SIZE)
		CASE(BODY_SUBGRAPH_INDEX)
		CASE(CELL_CLIP)
		CASE(COMBINER)
		CASE(COND_SUBGRAPH_INDEX)
		CASE(DEPTH_MULTIPLIER)
		CASE(DILATION_H_FACTOR)
		CASE(DILATION_W_FACTOR)
		CASE(ELLIPSIS_MASK)
		CASE(ELSE_SUBGRAPH_INDEX)
		CASE(EMBEDDING_DIM_PER_CHANNEL)
		CASE(END_MASK)
		CASE(FILTER_HEIGHT)
		CASE(FILTER_WIDTH)
		CASE(FUSED_ACTIVATION_FUNCTION)
		CASE(IDX_OUT_TYPE)
		CASE(IN_DATA_TYPE)
		CASE(INCLUDE_ALL_NGRAMS)
		CASE(KEEP_DIMS)
		CASE(KEEP_NUM_DIMS)
		CASE(KERNEL_TYPE)
		CASE(MAX)
		CASE(MAX_SKIP_SIZE)
		CASE(MERGE_OUTPUTS)
		CASE(MIN)
		CASE(MODE)
		CASE(NARROW_RANGE)
		CASE(NEW_AXIS_MASK)
		CASE(NEW_HEIGHT)
		CASE(NEW_SHAPE)
		CASE(NEW_WIDTH)
		CASE(NGRAM_SIZE)
		CASE(NUM)
		CASE(NUM_BITS)
		CASE(NUM_CHANNELS)
		CASE(NUM_COLUMNS_PER_CHANNEL)
		CASE(NUM_SPLITS)
		CASE(OUT_DATA_TYPE)
		CASE(OUT_TYPE)
		CASE(OUTPUT_TYPE)
		CASE(PADDING)
		CASE(PROJ_CLIP)
		CASE(RADIUS)
		CASE(RANK)
		CASE(SEQ_DIM)
		CASE(SHRINK_AXIS_MASK)
		CASE(SQUEEZE_DIMS)
		CASE(STRIDE_H)
		CASE(STRIDE_W)
		CASE(SUBGRAPH)
		CASE(THEN_SUBGRAPH_INDEX)
		CASE(TIME_MAJOR)
		CASE(TYPE)
		CASE(VALIDATE_INDICES)
		CASE(VALUES_COUNT)
		CASE(WEIGHTS_FORMAT)
	}
#undef CASE
	for (auto c : s)
		os << (char)std::tolower(c);
	return os;
}

std::ostream& operator<<(std::ostream &os, PluginInterface::OperatorOptionType optType) {
	switch (optType) {
	case PluginInterface::OperatorOption_TypeBool:
		os << "boolean";
		break;
	case PluginInterface::OperatorOption_TypeFloat:
		os << "float";
		break;
	case PluginInterface::OperatorOption_TypeInt:
		os << "int";
		break;
	case PluginInterface::OperatorOption_TypeUInt:
		os << "unsigned int";
		break;
	case PluginInterface::OperatorOption_TypeIntArray:
		os << "array of int";
		break;
	}
	return os;
}

std::ostream& operator<<(std::ostream &os, const PluginInterface::OperatorOptionValue &optValue) {
	switch (optValue.type) {
	case PluginInterface::OperatorOption_TypeBool:
		os << (optValue.b ? "true" : "false");
		break;
	case PluginInterface::OperatorOption_TypeFloat:
		os << optValue.f;
		break;
	case PluginInterface::OperatorOption_TypeInt:
		os << optValue.i;
		break;
	case PluginInterface::OperatorOption_TypeUInt:
		os << optValue.u;
		break;
	case PluginInterface::OperatorOption_TypeIntArray: {
		os << "{";
		int n = 0;
		for (auto i : optValue.ii)
			os << (n++ > 0 ? ", " : "") << i;
		os << "}";
		break;
	}}
	return os;
}
