#include "GraphicModel.h"

GraphicModel::GraphicModel(const ST::string path_, const uint16_t subImageIndex_) : path(path_), subImageIndex(subImageIndex_) {
}

const ST::string& GraphicModel::getPath() const {
	return this->path;
}

uint16_t GraphicModel::getSubImageIndex() const {
	return this->subImageIndex;
}

GraphicModel GraphicModel::deserialize(const JsonValue &json) {
	auto obj = json.toObject();
	return GraphicModel(
		obj.GetString("path"),
		obj.getOptionalUInt("subImageIndex", 0)
	);
}

JsonValue GraphicModel::serialize() const {
	JsonObject v;

	v.set("path", path);
	v.set("subImageIndex", subImageIndex);

	return v.toValue();
}


#ifdef WITH_UNITTESTS
#include "gtest/gtest.h"

// docs/item-graphic-frame-limits-plan.md, D3: pictures past 255 in a file
TEST(GraphicModel, subImageIndexAbove255)
{
	GraphicModel const g = GraphicModel::deserialize(JsonValue::deserialize(
		"{ \"path\": \"interface/mdguns.sti\", \"subImageIndex\": 300 }"));
	EXPECT_EQ(g.getSubImageIndex(), 300);
	EXPECT_EQ(GraphicModel::deserialize(g.serialize()).getSubImageIndex(), 300);
	EXPECT_EQ(GraphicModel::deserialize(JsonValue::deserialize(
		"{ \"path\": \"interface/mdguns.sti\" }")).getSubImageIndex(), 0);
}
#endif
