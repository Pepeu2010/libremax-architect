#pragma once
#include <QByteArray>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace lmx {
using Json = nlohmann::json;
struct Transform {
    double x = 0, y = 0, z = 0, yaw = 0;
    bool mirrored = false;
};
struct Entity {
    std::string id, type, name, parent;
    Transform transform;
    bool visible = true, locked = false;
    double width = 800, height = 720, depth = 550;
    std::string material = "white";
    Json parameters = Json::object();
    Json metadata = Json::object();
};
struct Document {
    std::string id, name = "Projeto sem título";
    int version = 1;
    std::vector<Entity> entities;
    Json materials;
    Json renderSettings = {
        {"camera", ""}, {"exposure", 0.0}, {"environmentStrength", 0.2}, {"denoise", true}};
    std::map<std::string, QByteArray> embeddedAssets;
    Document();
    Entity &at(const std::string &id);
    const Entity &at(const std::string &id) const;
    bool contains(const std::string &id) const;
    void validate() const;
    Json serialize() const;
    static Document deserialize(const Json &json);
};
std::string uuid();
double millimeters(double value);
Entity entity(std::string type, std::string name);
Entity wall(double x1, double y1, double x2, double y2, double height = 2700, double thickness = 120,
            bool half = false);
void addRectangularRoom(Document &document, double width, double depth, double height, double thickness,
                        double x = 0, double y = 0, const std::string &name = "Ambiente");
void eraseCascade(Document &document, const std::vector<std::string> &ids);
Json materialPresets();
} // namespace lmx
