

#include "../../plugin-interface.h"
#include "../../misc.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <memory>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <assert.h>
#include <errno.h>

#include "tflite_schema_generated.h" // a complete definition of TF Lite format based on the .fbs file from the TD Lite source tree

namespace Helpers {
	template<class C1, class C2>
	static void convertContainers(const C1 &src, C2 &dst) {
		for (auto c : src)
			dst.push_back(c);
	}
	const std::vector<int32_t> convertFlatbuffersIntListToStl(const flatbuffers::Vector<int> *lst) {
		std::vector<int32_t> v;
		convertContainers(*lst, v);
		return v;
	}

	static PluginInterface::OperatorKind opcodeToOperatorKind(tflite::BuiltinOperator opcode) {
		switch (opcode) {
#define CASE(hisName,myName) case tflite::BuiltinOperator_##hisName: return PluginInterface::Kind##myName;
#include "operator-list.cpp" // generated from schema.fbs
		default:
			FAIL("unknown opcode=" << opcode)
#undef CASE
		}
	}
	static PluginInterface::OperatorOptionsList* convertOperatorOptions(const tflite::Operator *o, tflite::BuiltinOperator opcode) {

		std::unique_ptr<PluginInterface::OperatorOptionsList> ourOpts(new PluginInterface::OperatorOptionsList);

		switch (opcode) {
#include "operator-options.cpp" // generated from schema.fbs
		default:
			// no nothing: no options for this operator
			;
		}

		return ourOpts.release();
	}
}

class TfLitePlugin : public PluginInterface {

	class Model : public PluginInterface::Model {
		const TfLitePlugin     *plugin;
		const tflite::SubGraph *subgraph; // the subgraoph that this model represents

		public:
			Model(const TfLitePlugin *plugin_, const tflite::SubGraph *subgraph_)
			: plugin(plugin_)
			, subgraph(subgraph_)
			{ }

		public: // interface
			unsigned numInputs() const override {
				return subgraph->inputs()->size();
			}
			std::vector<TensorId> getInputs() const override {
				std::vector<TensorId> idxs;
				Helpers::convertContainers(*subgraph->inputs(), idxs);
				return idxs;
			}
			unsigned numOutputs() const override {
				return subgraph->outputs()->size();
			}
			std::vector<TensorId> getOutputs() const override {
				std::vector<TensorId> idxs;
				Helpers::convertContainers(*subgraph->outputs(), idxs);
				return idxs;
			}
			unsigned numOperators() const override {
				return subgraph->operators()->size();
			}
			void getOperatorIo(OperatorId operatorId, std::vector<TensorId> &inputs, std::vector<TensorId> &outputs) const override {
				auto o = subgraph->operators()->Get(operatorId);
				Helpers::convertContainers(*o->inputs(), inputs);
				Helpers::convertContainers(*o->outputs(), outputs);
			}
			OperatorKind getOperatorKind(OperatorId operatorId) const override {
				auto opcode_index = subgraph->operators()->Get(operatorId)->opcode_index();
				assert(opcode_index < plugin->model->operator_codes()->size());
				return Helpers::opcodeToOperatorKind(plugin->model->operator_codes()->Get(opcode_index)->builtin_code());
			}
			PluginInterface::OperatorOptionsList* getOperatorOptions(OperatorId operatorId) const override {
				auto o = subgraph->operators()->Get(operatorId);
				return Helpers::convertOperatorOptions(o, plugin->model->operator_codes()->Get(o->opcode_index())->builtin_code());
			}
			unsigned numTensors() const override {
				return subgraph->tensors()->size();
			}
			TensorShape getTensorShape(TensorId tensorId) const override {
				std::vector<unsigned> shape;
				Helpers::convertContainers(*subgraph->tensors()->Get(tensorId)->shape(), shape);
				return shape;
			}
			std::string getTensorName(TensorId tensorId) const override {
				return subgraph->tensors()->Get(tensorId)->name()->c_str();
			}
			bool getTensorHasData(TensorId tensorId) const override {
				auto buffer = subgraph->tensors()->Get(tensorId)->buffer();
				assert(buffer < plugin->model->buffers()->size());
				return plugin->model->buffers()->Get(buffer)->data() != nullptr;
			}
			const float* getTensorData(TensorId tensorId) const override {
				auto buffer = subgraph->tensors()->Get(tensorId)->buffer();
				assert(buffer < plugin->model->buffers()->size());
				assert(plugin->model->buffers()->Get(buffer)->data() != nullptr);
				return (const float*)plugin->model->buffers()->Get(buffer)->data()->Data();
			}
			bool getTensorIsVariableFlag(TensorId tensorId) const override {
				return subgraph->tensors()->Get(tensorId)->is_variable();
			}
	};

	std::string                           modelFileName;
	int                                   fd;              // handle of the open file
	size_t                                fileSize;        // file size
	void*                                 mmappedPtr;
	const tflite::Model*                  model;
	std::string                           err;             // error message in case the error occurs
	std::unique_ptr<Model>                modelObj;        // the model that we own

public:
	TfLitePlugin()
	: fd(-1)
	, fileSize(0)
	, mmappedPtr(nullptr)
	, model(nullptr)
	{
	}
	~TfLitePlugin() {
		if (mmappedPtr)
			closeFileReleaseMemory();
	}


public: // interface implementation

	std::string filePath() const override {
		return modelFileName;
	}

	virtual bool open(const std::string &modelFileName_) override {
		// open the file
		fd = ::open(modelFileName_.c_str(), O_RDONLY); // since TF Lite models aren't officially writable we open it in RO mode
		if (fd == -1) {
			PRINT_ERR("failed to open the tflite file '" << modelFileName_ << "': " << strerror(errno))
			return false;
		}

		// find its size
		struct stat sb;
		if (::fstat(fd, &sb) == -1) {
			auto err = STR("failed to find the tflite file '" << modelFileName_ << "' length: " << strerror(errno));
			if (::close(fd) == -1)
				err += STR("; failed to close the tflite file '" << modelFileName_ << "': " << strerror(errno));
			PRINT_ERR(err)
			return false;
		}
		fileSize = sb.st_size;

		// mmap the file for efficient access
		void *m = ::mmap(0/*addr*/, sb.st_size, PROT_READ, MAP_SHARED/*flags*/, fd, 0/*offset*/);
		if (m == MAP_FAILED) {
			auto err = STR("failed to mmap the tflite file '" << modelFileName_ << "': " << strerror(errno));
			if (::close(fd) == -1)
				err += STR("; failed to close the tflite file '" << modelFileName_ << "': " << strerror(errno));
			PRINT_ERR(err)
			return false;
		}
		mmappedPtr = m;

		// view the memory as a model
		model = tflite::GetModel(mmappedPtr);

		// check if we can take this model
		if (model->subgraphs()->size() != 1) {
			PRINT_ERR("we only support TF Lite models with subgraph count of 1, the model '" << modelFileName_ << "' has " << model->subgraphs()->size() << " subgraphs")
			closeFileReleaseMemory();
			return false;
		}

		// return
		modelFileName = modelFileName_;
		modelObj.reset(new Model(this, model->subgraphs()->Get(0)));
		return true;
	}

	std::string errorMessage() const override {
		return "some strange error occurred"; // no error
	}
	size_t numModels() const override {
		return 1; // .tflite file always contains only one model
	}
	const Model* getModel(unsigned index) const override {
		// checks
		if (index != 0) {
			std::cerr << "ERROR only index=1 is available for TF Lite models" << std::endl;
			return nullptr;
		}
		if (modelObj.get() == nullptr) {
			std::cerr << "ERROR 'open' hasn't been called" << std::endl;
			return nullptr;
		}

		return modelObj.get();
	}

private:
	void closeFileReleaseMemory() {
		// delete the memory object
		modelFileName.clear();
		modelObj.reset(nullptr);
		model = nullptr;

		// unmap
		if (::munmap(mmappedPtr, fileSize) == -1)
			PRINT_ERR("failed to unmmap the tflite file '" << modelFileName << "': " << strerror(errno))
		mmappedPtr = nullptr;
		fileSize = 0;

		// close the file
		if (::close(fd) == -1)
			PRINT_ERR("failed to close the tflite file '" << modelFileName << "': " << strerror(errno))
		fd = -1;
	}
};


//
// exported function
//

extern "C" {

PluginInterface* createPluginInterface() {
	return new TfLitePlugin;
}

};

