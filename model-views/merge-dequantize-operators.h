// Copyright (C) 2020 by Yuri Victorovich. All rights reserved.

#pragma once

#include "../plugin-interface.h"

#include <memory>
#include <vector>

namespace ModelViews {

class MergeDequantizeOperators : public PluginInterface::Model {

// types
	typedef PluginInterface PI;

// data
	std::unique_ptr<const PluginInterface::Model> original;
	std::vector<PI::OperatorId>                   operatorMap; // view operator to original operator mapping
	std::vector<bool>                             tensorIsDequantizeInput;
	std::vector<bool>                             tensorIsDequantizeOutput;

public:
	MergeDequantizeOperators(const PluginInterface::Model *original_);

public: // interface implementation
	unsigned                    numInputs() const override;
	std::vector<PI::TensorId>   getInputs() const override;
	unsigned                    numOutputs() const override;
	std::vector<PI::TensorId>   getOutputs() const override;
	unsigned                    numOperators() const override;
	void                        getOperatorIo(unsigned operatorIdx, std::vector<PI::TensorId> &inputs, std::vector<PI::TensorId> &outputs) const override;
	PI::OperatorKind            getOperatorKind(unsigned operatorIdx) const override;
	PI::OperatorOptionsList*    getOperatorOptions(unsigned operatorIdx) const override;
	unsigned                    numTensors() const override;
	TensorShape                 getTensorShape(PI::TensorId tensorId) const override;
	std::string                 getTensorName(PI::TensorId tensorId) const override;
	bool                        getTensorHasData(PI::TensorId tensorId) const override;
	const float*                getTensorData(PI::TensorId tensorId) const override;
	bool                        getTensorIsVariableFlag(PI::TensorId tensorId) const override;

}; // MergeDequantize

} // ModelViews