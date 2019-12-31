
#include "model-functions.h"

#include <string>
#include <vector>
#include <set>
#include <sstream>

#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include "model-functions.h"
#include "plugin-interface.h"
#include "graphviz-cgraph.h"
#include "misc.h"
#include "util.h"

#include <QSizeF>
#include <QPointF>

// helper classes
class ConvStrToFloat {
public:
	static float conv(const std::string &str) {
		return std::stof(str);
	}
};

namespace ModelFunctions {

void renderModelToCoordinates(const PluginInterface::Model *model,
	const QMarginsF &operatorBoxMargins,
	std::function<QSizeF(PluginInterface::OperatorId)> operatorBoxFn, // operator boxes in inches
	Box2 &bbox,
	std::vector<Box2> &operatorBoxes,
	std::vector<std::vector<std::vector<QPointF>>> &tensorLineCubicSplines,
	std::vector<std::vector<QPointF>> &tensorLabelPositions
) {

	// map tensors to operators
	std::vector<int> tensorProducers;
	tensorProducers.resize(model->numTensors());
	std::vector<std::set<PluginInterface::OperatorId>> tensorConsumers;
	tensorConsumers.resize(model->numTensors());
	{
		for (auto &p : tensorProducers)
			p = -1;
		for (PluginInterface::OperatorId oid = 0, oide = (PluginInterface::OperatorId)model->numOperators(); oid < oide; oid++) {
			std::vector<PluginInterface::TensorId> oinputs, ooutputs;
			model->getOperatorIo(oid, oinputs, ooutputs);
			for (auto o : ooutputs)
				tensorProducers[o] = oid;
			for (auto i : oinputs)
				tensorConsumers[i].insert(oid);
		}
	}

	// build the graphviz graph

	Graphviz_CGraph graph("NnModel");
	graph.setGraphDpi(Util::getScreenDPI());
	graph.setGraphOrdering(true/*orderingIn*/, false/*orderingOut*/); // preserve edge order as they are consumed as inputs.
	                                                                  // ideally we need to have both "in" and "out" ordering
	                                                                  // see https://gitlab.com/graphviz/graphviz/issues/1645
	graph.setDefaultNodeShape("box");
	graph.setDefaultNodeSize(0,0);

	std::vector<Graphviz_CGraph::Node> operatorNodes;
	for (PluginInterface::OperatorId oid = 0, oide = model->numOperators(); oid < oide; oid++) {
		auto node = graph.addNode(CSTR("Op_" << oid));
		auto szBox = operatorBoxFn(oid);
		graph.setNodeSize(node,
			operatorBoxMargins.left() + szBox.width() + operatorBoxMargins.right(),
			operatorBoxMargins.top() + szBox.height() + operatorBoxMargins.bottom()
		);
		operatorNodes.push_back(node);
	}

	std::vector<std::vector<Graphviz_CGraph::Node>> tensorEdges;
	tensorEdges.resize(model->numTensors());
	for (PluginInterface::TensorId tid = 0, tide = model->numTensors(); tid < tide; tid++)
		if (tensorProducers[tid] != -1 && !tensorConsumers[tid].empty())
			for (auto oidConsumer : tensorConsumers[tid]) {
				auto edge = graph.addEdge(operatorNodes[tensorProducers[tid]], operatorNodes[oidConsumer], ""/*name(key)*/);
				graph.setEdgeLabel(edge, CSTR(model->getTensorShape(tid)));
				tensorEdges[tid].push_back(edge);
			}

	// render the graph

	graph.render();

	// helpers
	auto posToQPointF = [](const std::array<float,2> &pos) {
		return QPointF(pos[0], pos[1]);
	};
	auto parseSplines = [](const std::string &splines) {
		std::vector<std::string> strpts;
		Util::splitString(splines, strpts, ' ');
		assert(strpts.size()%3 == 2); // n = 1 (mod 3) for splines, and the e,Pt endpoint

		std::vector<QPointF> pts;
		QPointF endp;
		for (auto &s : strpts)
			if (s[0] != 'e') { // ignore it for now
				assert(s[0] != 's'); // we don't yet support startp (if startp is not given, p1 touches a node)
				std::vector<float> ptFloats;
				Util::splitString<std::vector<float>, ConvStrToFloat>(s, ptFloats, ',');
				assert(ptFloats.size() == 2);
				pts.push_back(QPointF(ptFloats[0], ptFloats[1]));
			} else { // endp
				std::vector<float> ptFloats;
				Util::splitString<std::vector<float>, ConvStrToFloat>(s.substr(2), ptFloats, ',');
				assert(ptFloats.size() == 2);
				endp = QPointF(ptFloats[0], ptFloats[1]);
			}
		pts.push_back(endp);
		return pts;
	};

	// bbox
	bbox = graph.getBBox();

	// operators (nodes)
	operatorBoxes.clear();
	for (auto node : operatorNodes)
		operatorBoxes.push_back(Box2{{graph.getNodePos(node), graph.getNodeSize(node)}});

	// tensors (edges)
	tensorLabelPositions.resize(tensorEdges.size());
	tensorLineCubicSplines.resize(tensorEdges.size());
	for (PluginInterface::TensorId tid = 0, tide = tensorEdges.size(); tid<tide; tid++)
		for (auto edge : tensorEdges[tid]) {
			// splines
			tensorLineCubicSplines[tid].push_back(parseSplines(graph.getEdgeSplines(edge)));
			// label position
			tensorLabelPositions[tid].push_back(posToQPointF(graph.getEdgeLabelPosition(edge)));
		}
}

}
