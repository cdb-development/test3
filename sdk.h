#pragma once
#include <Windows.h>
#include <cmath>
#include <vector>
#include <string>
#include <map>
#include "driver.h"
#include "offsets.h"
#include "settings.h"
#include <d3d9types.h>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif


namespace DBDKillers {
    const std::map<int, std::string> Data = {
        {268435456, "The Trapper"},
        {268435457, "The Wraith"},
        {268435458, "The Hillbilly"},
        {268435459, "The Nurse"},
        {268435460, "The Hag"},
        {268435461, "The Shape"},
        {268435462, "The Doctor"},
        {268435463, "The Huntress"},
        {268435464, "The Cannibal"},
        {268435465, "The Nightmare"},
        {268435466, "The Pig"},
        {268435467, "The Clown"},
        {268435468, "The Spirit"},
        {268435469, "The Legion"},
        {268435470, "The Plague"},
        {268435471, "The Ghost Face"},
        {268435472, "The Demogorgon"},
        {268435473, "The Oni"},
        {268435474, "The Deathslinger"},
        {268435475, "The Executioner"},
        {268435476, "The Blight"},
        {268435477, "The Twins"},
        {268435478, "The Trickster"},
        {268435479, "The Nemesis"},
        {268435480, "The Cenobite"},
        {268435481, "The Artist"},
        {268435482, "The Onryō"},
        {268435483, "The Dredge"},
        {268435484, "The Mastermind"},
        {268435485, "The Knight"},
        {268435486, "The Skull Merchant"},
        {268435487, "The Singularity"},
        {268435488, "The Xenomorph"},
        {268435489, "The Good Guy"},
        {268435490, "The Unknown"},
        {268435491, "The Lich"},
        {268435492, "Dracula"},
        {268435493, "The Houndmaster"}
    };

    inline std::string GetKillerNameById(int killerId) {
        auto it = Data.find(killerId);
        return (it != Data.end()) ? it->second : "Unknown Killer";
    }

    inline int GetKillerIdByName(const std::string& killerName) {
        for (const auto& pair : Data) {
            if (pair.second == killerName) {
                return pair.first;
            }
        }
        return 0; 
    }

    inline bool IsValidKillerId(int killerId) {
        return Data.find(killerId) != Data.end();
    }
}

struct vec2 {
    float x, y;
};

struct Rotator {
    double Pitch, Yaw, Roll;
};

struct Vector3 {
    double x, y, z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(double x, double y, double z) : x(x), y(y), z(z) {}

    Vector3 operator-(const Vector3& ape) const {
        return { x - ape.x, y - ape.y, z - ape.z };
    }

    Vector3 operator+(const Vector3& ape) const {
        return { x + ape.x, y + ape.y, z + ape.z };
    }

    Vector3 operator*(double ape) const {
        return { x * ape, y * ape, z * ape };
    }

    Vector3 operator/(double ape) const {
        return { x / ape, y / ape, z / ape };
    }

    Vector3& operator/=(double ape) {
        x /= ape;
        y /= ape;
        z /= ape;
        return *this;
    }

    Vector3& operator+=(const Vector3& ape) {
        x += ape.x;
        y += ape.y;
        z += ape.z;
        return *this;
    }

    Vector3& operator-=(const Vector3& ape) {
        x -= ape.x;
        y -= ape.y;
        z -= ape.z;
        return *this;
    }

    double Length() const {
        return sqrt((x * x) + (y * y) + (z * z));
    }

    double Length2D() const {
        return sqrt((x * x) + (y * y));
    }

    double DistTo(const Vector3& ape) const {
        return (*this - ape).Length();
    }

    double Dist2D(const Vector3& ape) const {
        return (*this - ape).Length2D();
    }

    double Dot(const Vector3& v) const {
        return x * v.x + y * v.y + z * v.z;
    }

    bool IsValid() const {
        return x != 0.0 || y != 0.0 || z != 0.0;
    }
    bool IsValidScreenCoord() const {
        return std::isfinite(x) && std::isfinite(y) && std::isfinite(z) &&
            x > -99999.0 && x < 99999.0 && y > -99999.0 && y < 99999.0;
    }
};

struct Vector4 {
    double w, x, y, z;
};

struct FMatrix {
    struct FPlane {
        float x, y, z, w;
        FPlane() : x(0), y(0), z(0), w(0) {}
        FPlane(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    };

    FPlane XPlane, YPlane, ZPlane, WPlane;

    FMatrix() {
        XPlane = FPlane(1, 0, 0, 0);
        YPlane = FPlane(0, 1, 0, 0);
        ZPlane = FPlane(0, 0, 1, 0);
        WPlane = FPlane(0, 0, 0, 1);
    }

    FMatrix(const FPlane& x, const FPlane& y, const FPlane& z, const FPlane& w)
        : XPlane(x), YPlane(y), ZPlane(z), WPlane(w) {
    }

    D3DMATRIX ToD3DMatrix() const {
        D3DMATRIX m;
        m._11 = XPlane.x; m._12 = XPlane.y; m._13 = XPlane.z; m._14 = XPlane.w;
        m._21 = YPlane.x; m._22 = YPlane.y; m._23 = YPlane.z; m._24 = YPlane.w;
        m._31 = ZPlane.x; m._32 = ZPlane.y; m._33 = ZPlane.z; m._34 = ZPlane.w;
        m._41 = WPlane.x; m._42 = WPlane.y; m._43 = WPlane.z; m._44 = WPlane.w;
        return m;
    }

    FMatrix operator*(const FMatrix& other) const {
        FMatrix result;

        result.XPlane.x = XPlane.x * other.XPlane.x + XPlane.y * other.YPlane.x + XPlane.z * other.ZPlane.x + XPlane.w * other.WPlane.x;
        result.XPlane.y = XPlane.x * other.XPlane.y + XPlane.y * other.YPlane.y + XPlane.z * other.ZPlane.y + XPlane.w * other.WPlane.y;
        result.XPlane.z = XPlane.x * other.XPlane.z + XPlane.y * other.YPlane.z + XPlane.z * other.ZPlane.z + XPlane.w * other.WPlane.z;
        result.XPlane.w = XPlane.x * other.XPlane.w + XPlane.y * other.YPlane.w + XPlane.z * other.ZPlane.w + XPlane.w * other.WPlane.w;

        result.YPlane.x = YPlane.x * other.XPlane.x + YPlane.y * other.YPlane.x + YPlane.z * other.ZPlane.x + YPlane.w * other.WPlane.x;
        result.YPlane.y = YPlane.x * other.XPlane.y + YPlane.y * other.YPlane.y + YPlane.z * other.ZPlane.y + YPlane.w * other.WPlane.y;
        result.YPlane.z = YPlane.x * other.XPlane.z + YPlane.y * other.YPlane.z + YPlane.z * other.ZPlane.z + YPlane.w * other.WPlane.z;
        result.YPlane.w = YPlane.x * other.XPlane.w + YPlane.y * other.YPlane.w + YPlane.z * other.ZPlane.w + YPlane.w * other.WPlane.w;

        result.ZPlane.x = ZPlane.x * other.XPlane.x + ZPlane.y * other.YPlane.x + ZPlane.z * other.ZPlane.x + ZPlane.w * other.WPlane.x;
        result.ZPlane.y = ZPlane.x * other.XPlane.y + ZPlane.y * other.YPlane.y + ZPlane.z * other.ZPlane.y + ZPlane.w * other.WPlane.y;
        result.ZPlane.z = ZPlane.x * other.XPlane.z + ZPlane.y * other.YPlane.z + ZPlane.z * other.ZPlane.z + ZPlane.w * other.WPlane.z;
        result.ZPlane.w = ZPlane.x * other.XPlane.w + ZPlane.y * other.YPlane.w + ZPlane.z * other.ZPlane.w + ZPlane.w * other.WPlane.w;

        result.WPlane.x = WPlane.x * other.XPlane.x + WPlane.y * other.YPlane.x + WPlane.z * other.ZPlane.x + WPlane.w * other.WPlane.x;
        result.WPlane.y = WPlane.x * other.XPlane.y + WPlane.y * other.YPlane.y + WPlane.z * other.ZPlane.y + WPlane.w * other.WPlane.y;
        result.WPlane.z = WPlane.x * other.XPlane.z + WPlane.y * other.YPlane.z + WPlane.z * other.ZPlane.z + WPlane.w * other.WPlane.z;
        result.WPlane.w = WPlane.x * other.XPlane.w + WPlane.y * other.YPlane.w + WPlane.z * other.ZPlane.w + WPlane.w * other.WPlane.w;

        return result;
    }

    Vector3 TransformVector(const Vector3& v) const {
        return Vector3(
            XPlane.x * v.x + YPlane.x * v.y + ZPlane.x * v.z + WPlane.x,
            XPlane.y * v.x + YPlane.y * v.y + ZPlane.y * v.z + WPlane.y,
            XPlane.z * v.x + YPlane.z * v.y + ZPlane.z * v.z + WPlane.z
        );
    }
};

// Game structures
typedef struct _EntityList {
    uintptr_t instance;
    uintptr_t mesh;
    uintptr_t root_component;
    uintptr_t instigator;
    uintptr_t PlayerState;
    uintptr_t Pawn;
    Vector3 TopLocation;
    Vector3 bone_origin;
    std::string name;
    std::string actorName; // Original Actor Name hinzufügen

    Vector3 origin;
    float health;
    float dist;
    int objectId;
    int team;
} EntityList;

struct FMinimalViewInfo {
    Vector3 Location;
    Vector3 Rotation;
    float FOV;
};

struct FCameraCacheEntry {
    float Timestamp;
    char pad_4[0xc];
    FMinimalViewInfo POV;
};

class UPlayer {
public:
    uintptr_t instance;
    uintptr_t mesh;
    uintptr_t root_component;
    uintptr_t OutlineComponent;
    uintptr_t instigator;
    uintptr_t PlayerState;
    uintptr_t Pawn;
    Vector3 TopLocation;
    std::string name;
    int objectId;
    Vector3 origin;
    float health;
    float dist;
    int team;
};

struct FTransform {
    Vector4 rot;
    Vector3 translation;
    char pad[4];
    Vector3 scale;
    char pad1[4];

    D3DMATRIX ToMatrixWithScale() {
        D3DMATRIX m;
        m._41 = translation.x;
        m._42 = translation.y;
        m._43 = translation.z;

        float x2 = rot.x + rot.x;
        float y2 = rot.y + rot.y;
        float z2 = rot.z + rot.z;

        float xx2 = rot.x * x2;
        float yy2 = rot.y * y2;
        float zz2 = rot.z * z2;
        m._11 = (1.0f - (yy2 + zz2)) * scale.x;
        m._22 = (1.0f - (xx2 + zz2)) * scale.y;
        m._33 = (1.0f - (xx2 + yy2)) * scale.z;

        float yz2 = rot.y * z2;
        float wx2 = rot.w * x2;
        m._32 = (yz2 - wx2) * scale.z;
        m._23 = (yz2 + wx2) * scale.y;

        float xy2 = rot.x * y2;
        float wz2 = rot.w * z2;
        m._21 = (xy2 - wz2) * scale.y;
        m._12 = (xy2 + wz2) * scale.x;

        float xz2 = rot.x * z2;
        float wy2 = rot.w * y2;
        m._31 = (xz2 + wy2) * scale.z;
        m._13 = (xz2 - wy2) * scale.x;

        m._14 = 0.0f;
        m._24 = 0.0f;
        m._34 = 0.0f;
        m._44 = 1.0f;

        return m;
    }
};

struct KillerAura {
    Vector3 position;
    float intensity;
    ULONGLONG lastUpdate;
    bool isActive;
    std::string killerName;
    int killerId;
    uintptr_t killerMesh;

    KillerAura() : position(0, 0, 0), intensity(1.0f),
        lastUpdate(0), isActive(false), killerName(""), killerId(0), killerMesh(0) {
    }
};


inline KillerAura g_KillerAura;
inline std::vector<Vector3> g_KillerTrail;
inline ULONGLONG g_LastKillerSeen = 0;
inline const ULONGLONG KILLER_MEMORY_TIME = 30000;

inline D3DMATRIX CreateMatrix(Vector3 rot, Vector3 origin);

inline Vector3 ClampToScreen(const Vector3& screenPos, bool allowOffscreen = false) {
    Vector3 result = screenPos;

    if (!allowOffscreen) {
        result.x = std::max(-50.0, std::min((double)(globals.ScreenWidth + 50), result.x));
        result.y = std::max(-50.0, std::min((double)(globals.ScreenHeight + 50), result.y));
    }
    else {
        result.x = std::max(-500.0, std::min((double)(globals.ScreenWidth + 500), result.x));
        result.y = std::max(-500.0, std::min((double)(globals.ScreenHeight + 500), result.y));
    }
    if (!std::isfinite(result.x)) result.x = globals.ScreenWidth / 2.0;
    if (!std::isfinite(result.y)) result.y = globals.ScreenHeight / 2.0;
    if (!std::isfinite(result.z)) result.z = 1.0;

    return result;
}

inline Vector3 WorldToScreenSafe(FMinimalViewInfo camera, Vector3 WorldLocation, bool allowOffscreen = false) {
    Vector3 Screenlocation(0, 0, 0);

    try {
        if (!WorldLocation.IsValid() || !camera.Location.IsValid()) {
            return Vector3(globals.ScreenWidth / 2.0, globals.ScreenHeight / 2.0, -1.0);
        }

        const D3DMATRIX tempMatrix = CreateMatrix(camera.Rotation, Vector3(0, 0, 0));

        Vector3 vAxisX(tempMatrix.m[0][0], tempMatrix.m[0][1], tempMatrix.m[0][2]);
        Vector3 vAxisY(tempMatrix.m[1][0], tempMatrix.m[1][1], tempMatrix.m[1][2]);
        Vector3 vAxisZ(tempMatrix.m[2][0], tempMatrix.m[2][1], tempMatrix.m[2][2]);

        Vector3 vDelta = WorldLocation - camera.Location;
        Vector3 vTransformed = Vector3(vDelta.Dot(vAxisY), vDelta.Dot(vAxisZ), vDelta.Dot(vAxisX));

        const float MIN_Z = 0.1f;
        const float MAX_Z = 50000.0f;

        if (vTransformed.z < MIN_Z) {
            if (allowOffscreen) {
                vTransformed.z = MIN_Z;
            }
            else {
                return Vector3(globals.ScreenWidth / 2.0, globals.ScreenHeight / 2.0, -1.0);
            }
        }

        if (vTransformed.z > MAX_Z) {
            vTransformed.z = MAX_Z;
        }

        float fov = camera.FOV;
        if (fov < 30.0f) fov = 90.0f;
        if (fov > 180.0f) fov = 120.0f;

        const float FOV_DEG_TO_RAD = static_cast<float>(3.14159265358979323846) / 360.f;
        const float fovRadians = fov * FOV_DEG_TO_RAD;
        const float tanFov = tanf(fovRadians);

        if (tanFov < 0.001f) {
            return Vector3(globals.ScreenWidth / 2.0, globals.ScreenHeight / 2.0, -1.0);
        }

        const float screenCenterX = globals.ScreenWidth / 2.0f;
        const float screenCenterY = globals.ScreenHeight / 2.0f;
        const float projectionScale = screenCenterX / tanFov;

        Screenlocation.x = screenCenterX + (vTransformed.x * projectionScale) / vTransformed.z;
        Screenlocation.y = screenCenterY - (vTransformed.y * projectionScale) / vTransformed.z;
        Screenlocation.z = vTransformed.z;

        return ClampToScreen(Screenlocation, allowOffscreen);

    }
    catch (...) {
        return Vector3(globals.ScreenWidth / 2.0, globals.ScreenHeight / 2.0, -1.0);
    }
}

inline Vector3 WorldToScreen(FMinimalViewInfo camera, Vector3 WorldLocation) {
    return WorldToScreenSafe(camera, WorldLocation, false);
}

inline Vector3 WorldToScreenNoOcclusion(FMinimalViewInfo camera, Vector3 WorldLocation) {
    return WorldToScreenSafe(camera, WorldLocation, true);
}


inline void UpdateKillerAura(const Vector3& killerPos, const std::string& name, int id, uintptr_t mesh = 0) {
    g_KillerAura.position = killerPos;
    g_KillerAura.killerName = name;
    g_KillerAura.killerId = id;
    g_KillerAura.killerMesh = mesh;
    g_KillerAura.isActive = true;
    g_KillerAura.lastUpdate = GetTickCount64();
    g_LastKillerSeen = GetTickCount64();
    g_KillerTrail.push_back(killerPos);
    if (g_KillerTrail.size() > 20) {
    g_KillerTrail.erase(g_KillerTrail.begin());
    }
}

inline bool IsKillerEntity(const std::string& actorName) {
    return actorName.find("BP_Slasher") != std::string::npos;
}

inline std::string GetSurvivorName(const std::string& actorName) {
    static const std::vector<std::pair<std::string, std::string>> survivors = {
        {"BP_CamperMale01", "Dwight Fairfield"},
        {"BP_CamperFemale01", "Meg Thomas"},
        {"BP_CamperMale02", "Jake Park"},
        {"BP_CamperFemale02", "Claudette Morel"},
        {"BP_CamperFemale03", "Nea Karlsson"},
        {"BP_CamperMale03", "David King"},
        {"BP_CamperFemale04", "Laurie Strode"},
        {"BP_CamperMale04", "Bill Overbeck"},
        {"BP_CamperMale05", "Detective David Tapp"},
        {"BP_CamperFemale05", "Feng Min"},
        {"BP_CamperMale06", "Quentin Smith"},
        {"BP_CamperFemale06", "Kate Denson"},
        {"BP_CamperMale07", "Adam Francis"},
        {"BP_CamperFemale07", "Jane Romero"},
        {"BP_CamperMale08", "Jeff Johansen"},
        {"BP_CamperFemale08", "Élodie Rakoto"},
        {"BP_CamperMale09", "Felix Richter"},
        {"BP_CamperFemale09", "Mikaela Reid"},
        {"BP_CamperMale10", "Jonah Vasquez"},
        {"BP_CamperFemale10", "Yui Kimura"},
        {"BP_CamperFemale11", "Zarina Kassir"},
        {"BP_CamperFemale12", "Cheryl Mason"},
        {"BP_CamperFemale13", "Jill Valentine"},
        {"BP_CamperMale11", "Leon S. Kennedy"},
        {"BP_CamperFemale14", "Yun-Jin Lee"},
        {"BP_CamperMale12", "Steve Harrington"},
        {"BP_CamperFemale15", "Nancy Wheeler"},
        {"BP_CamperMale13", "Ash J. Williams"},
        {"BP_CamperFemale16", "Rebecca Chambers"},
        {"BP_CamperMale14", "Vittorio Toscano"},
        {"BP_CamperFemale17", "Thalita Lyra"},
        {"BP_CamperFemale18", "Haddie Kaur"},
        {"BP_CamperFemale19", "Ada Wong"},
        {"BP_CamperMale15", "Alan Wake"},
        {"BP_CamperFemale20", "Aestri Yazar"},
        {"BP_CamperMale16", "Yoichi Asakawa"},
        {"BP_CamperMale17", "Ace Visconti"},
        {"BP_CamperFemale21", "Orela Rose"},
        {"BP_CamperMale18", "Trevor"},
        {"BP_CamperFemale22", "Vee Boonyasak"},

        {"Camper_Male01", "Dwight Fairfield"},
        {"Camper_Female01", "Meg Thomas"},
        {"Camper_Male02", "Jake Park"},
        {"Camper_Female02", "Claudette Morel"},
        {"Camper_Female03", "Nea Karlsson"},
        {"Camper_Male03", "David King"},
        {"Camper_Female04", "Laurie Strode"},
        {"Camper_Male04", "Bill Overbeck"},
        {"Camper_Male05", "Detective David Tapp"},
        {"Camper_Female05", "Feng Min"},
        {"Camper_Male06", "Quentin Smith"},
        {"Camper_Female06", "Kate Denson"},
        {"Camper_Male07", "Adam Francis"},
        {"Camper_Female07", "Jane Romero"},
        {"Camper_Male08", "Jeff Johansen"},
        {"Camper_Female08", "Élodie Rakoto"},
        {"Camper_Male09", "Felix Richter"},
        {"Camper_Female09", "Mikaela Reid"},
        {"Camper_Male10", "Jonah Vasquez"},
        {"Camper_Female10", "Yui Kimura"},
        {"Camper_Female11", "Zarina Kassir"},
        {"Camper_Female12", "Cheryl Mason"},
        {"Camper_Female13", "Jill Valentine"},
        {"Camper_Male11", "Leon S. Kennedy"},
        {"Camper_Female14", "Yun-Jin Lee"},
        {"Camper_Male12", "Steve Harrington"},
        {"Camper_Female15", "Nancy Wheeler"},
        {"Camper_Male13", "Ash J. Williams"},
        {"Camper_Female16", "Rebecca Chambers"},
        {"Camper_Male14", "Vittorio Toscano"},
        {"Camper_Female17", "Thalita Lyra"},
        {"Camper_Female18", "Haddie Kaur"},
        {"Camper_Female19", "Ada Wong"},
        {"Camper_Male15", "Alan Wake"},
        {"Camper_Female20", "Aestri Yazar"},
        {"Camper_Male16", "Yoichi Asakawa"},
        {"Camper_Male17", "Ace Visconti"},
        {"Camper_Female21", "Orela Rose"},
        {"Camper_Male18", "Trevor"},
        {"Camper_Female22", "Vee Boonyasak"},

        {"Character01", "Dwight Fairfield"},
        {"Character02", "Meg Thomas"},
        {"Character03", "Jake Park"},
        {"Character04", "Claudette Morel"},
        {"Character05", "Nea Karlsson"},
        {"Character06", "David King"},
        {"Character07", "Laurie Strode"},
        {"Character08", "Bill Overbeck"},
        {"Character09", "Detective David Tapp"},
        {"Character10", "Feng Min"},
        {"Character11", "Quentin Smith"},
        {"Character12", "Kate Denson"},
        {"Character13", "Adam Francis"},
        {"Character14", "Jane Romero"},
        {"Character15", "Jeff Johansen"},
        {"Character16", "Élodie Rakoto"},
        {"Character17", "Felix Richter"},
        {"Character18", "Mikaela Reid"},
        {"Character19", "Jonah Vasquez"},
        {"Character20", "Yui Kimura"},
        {"Character21", "Zarina Kassir"},
        {"Character22", "Cheryl Mason"},
        {"Character23", "Jill Valentine"},
        {"Character24", "Leon S. Kennedy"},
        {"Character25", "Yun-Jin Lee"},
        {"Character26", "Steve Harrington"},
        {"Character27", "Nancy Wheeler"},
        {"Character28", "Ash J. Williams"},
        {"Character29", "Rebecca Chambers"},
        {"Character30", "Vittorio Toscano"},
        {"Character31", "Thalita Lyra"},
        {"Character32", "Haddie Kaur"},
        {"Character33", "Ada Wong"},
        {"Character34", "Alan Wake"},
        {"Character35", "Aestri Yazar"},
        {"Character36", "Yoichi Asakawa"},
        {"Character37", "Ace Visconti"},
        {"Character38", "Orela Rose"},
        {"Character39", "Trevor"},
        {"Character40", "Vee Boonyasak"},

        {"Steve Harrington", "Steve Harrington"},
        {"Leon S. Kennedy", "Leon S. Kennedy"},
        {"Ash J. Williams", "Ash J. Williams"},
        {"Detective David Tapp", "Detective David Tapp"},
        {"Dwight Fairfield", "Dwight Fairfield"},
        {"Claudette Morel", "Claudette Morel"},
        {"Laurie Strode", "Laurie Strode"},
        {"Bill Overbeck", "Bill Overbeck"},
        {"Quentin Smith", "Quentin Smith"},
        {"Kate Denson", "Kate Denson"},
        {"Adam Francis", "Adam Francis"},
        {"Jane Romero", "Jane Romero"},
        {"Jeff Johansen", "Jeff Johansen"},
        {"Élodie Rakoto", "Élodie Rakoto"},
        {"Felix Richter", "Felix Richter"},
        {"Mikaela Reid", "Mikaela Reid"},
        {"Jonah Vasquez", "Jonah Vasquez"},
        {"Nea Karlsson", "Nea Karlsson"},
        {"David King", "David King"},
        {"Meg Thomas", "Meg Thomas"},
        {"Jake Park", "Jake Park"},
        {"Yui Kimura", "Yui Kimura"},
        {"Zarina Kassir", "Zarina Kassir"},
        {"Cheryl Mason", "Cheryl Mason"},
        {"Jill Valentine", "Jill Valentine"},
        {"Yun-Jin Lee", "Yun-Jin Lee"},
        {"Nancy Wheeler", "Nancy Wheeler"},
        {"Rebecca Chambers", "Rebecca Chambers"},
        {"Vittorio Toscano", "Vittorio Toscano"},
        {"Thalita Lyra", "Thalita Lyra"},
        {"Haddie Kaur", "Haddie Kaur"},
        {"Ada Wong", "Ada Wong"},
        {"Alan Wake", "Alan Wake"},
        {"Aestri Yazar", "Aestri Yazar"},
        {"Yoichi Asakawa", "Yoichi Asakawa"},
        {"Ace Visconti", "Ace Visconti"},
        {"Orela Rose", "Orela Rose"},
        {"Vee Boonyasak", "Vee Boonyasak"},

        {"Dwight", "Dwight Fairfield"},
        {"Claudette", "Claudette Morel"},
        {"Laurie", "Laurie Strode"},
        {"Quentin", "Quentin Smith"},
        {"Adam", "Adam Francis"},
        {"Jane", "Jane Romero"},
        {"Jeff", "Jeff Johansen"},
        {"Elodie", "Élodie Rakoto"},
        {"Felix", "Felix Richter"},
        {"Mikaela", "Mikaela Reid"},
        {"Jonah", "Jonah Vasquez"},
        {"Zarina", "Zarina Kassir"},
        {"Cheryl", "Cheryl Mason"},
        {"Jill", "Jill Valentine"},
        {"YunJin", "Yun-Jin Lee"},
        {"Nancy", "Nancy Wheeler"},
        {"Rebecca", "Rebecca Chambers"},
        {"Vittorio", "Vittorio Toscano"},
        {"Thalita", "Thalita Lyra"},
        {"Haddie", "Haddie Kaur"},
        {"Alan", "Alan Wake"},
        {"Aestri", "Aestri Yazar"},
        {"Yoichi", "Yoichi Asakawa"},
        {"Orela", "Orela Rose"},
        {"Trevor", "Trevor"},
        {"Bill", "Bill Overbeck"},
        {"Tapp", "Detective David Tapp"},
        {"Feng", "Feng Min"},
        {"Kate", "Kate Denson"},
        {"Yui", "Yui Kimura"},
        {"Ada", "Ada Wong"},
        {"Ace", "Ace Visconti"},
        {"Vee", "Vee Boonyasak"},
        {"Nea", "Nea Karlsson"},
        {"David", "David King"},
        {"Jake", "Jake Park"},
        {"Meg", "Meg Thomas"},
        {"Ash", "Ash J. Williams"},
        {"Leon", "Leon S. Kennedy"},
        {"Steve", "Steve Harrington"}
    };
    for (const auto& survivor : survivors) {
        if (actorName == survivor.first) {
            return survivor.second;
        }
    }
    for (const auto& survivor : survivors) {
        if (actorName.find(survivor.first) != std::string::npos) {
            return survivor.second;
        }
    }

    return "Unknown Survivor";
}

inline std::string GetKillerNameFromActor(const std::string& actorName) {
    if (actorName.find("Character_01") != std::string::npos) return "The Trapper";
    else if (actorName.find("Character_02") != std::string::npos) return "The Wraith";
    else if (actorName.find("Character_03") != std::string::npos) return "The Hillbilly";
    else if (actorName.find("Character_04") != std::string::npos) return "The Nurse";
    else if (actorName.find("Character_05") != std::string::npos) return "The Shape";
    else if (actorName.find("Character_06") != std::string::npos) return "The Hag";
    else if (actorName.find("Character_07") != std::string::npos) return "The Doctor";
    else if (actorName.find("Character_08") != std::string::npos) return "The Huntress";
    else if (actorName.find("Character_09") != std::string::npos) return "The Cannibal";
    else if (actorName.find("Character_10") != std::string::npos) return "The Nightmare";
    else if (actorName.find("Character_11") != std::string::npos) return "The Pig";
    else if (actorName.find("Character_12") != std::string::npos) return "The Clown";
    else if (actorName.find("Character_13") != std::string::npos) return "The Spirit";
    else if (actorName.find("Character_14") != std::string::npos) return "The Legion";
    else if (actorName.find("Character_15") != std::string::npos) return "The Plague";
    else if (actorName.find("Character_16") != std::string::npos) return "The Ghost Face";
    else if (actorName.find("Character_17") != std::string::npos) return "The Demogorgon";
    else if (actorName.find("Character_18") != std::string::npos) return "The Oni";
    else if (actorName.find("Character_19") != std::string::npos) return "The Deathslinger";
    else if (actorName.find("Character_20") != std::string::npos) return "The Executioner";
    else if (actorName.find("Character_21") != std::string::npos) return "The Nemesis";
    else if (actorName.find("Character_22") != std::string::npos) return "The Cenobite";
    else if (actorName.find("Character_23") != std::string::npos) return "The Artist";
    else if (actorName.find("Character_24") != std::string::npos) return "The Onryo";
    else if (actorName.find("Character_25") != std::string::npos) return "The Dredge";
    else if (actorName.find("Character_26") != std::string::npos) return "The Mastermind";
    else if (actorName.find("Character_27") != std::string::npos) return "The Knight";
    else if (actorName.find("Character_28") != std::string::npos) return "The Skull Merchant";
    else if (actorName.find("Character_29") != std::string::npos) return "The Singularity";
    else if (actorName.find("Character_30") != std::string::npos) return "The Xenomorph";
    else if (actorName.find("Character_31") != std::string::npos) return "The Good Guy";
    else if (actorName.find("Character_32") != std::string::npos) return "The Unknown";
    else if (actorName.find("Character_33") != std::string::npos) return "The Lich";
    else if (actorName.find("Character_34") != std::string::npos) return "Dracula";
    else if (actorName.find("Character_35") != std::string::npos) return "The Houndmaster";
    else if (actorName.find("Character_36") != std::string::npos) return "The Ghoul";
    else if (actorName.find("Character_37") != std::string::npos) return "The Animatronic";
    else if (actorName.find("Character_38") != std::string::npos) return "The Krasue";
    else return "Unknown Killer";
}



inline float ToMeters(float x) {
    return x / 39.62f;
}

inline D3DMATRIX MatrixMultiplication(D3DMATRIX pM1, D3DMATRIX pM2) {
    D3DMATRIX pOut;
    pOut._11 = pM1._11 * pM2._11 + pM1._12 * pM2._21 + pM1._13 * pM2._31 + pM1._14 * pM2._41;
    pOut._12 = pM1._11 * pM2._12 + pM1._12 * pM2._22 + pM1._13 * pM2._32 + pM1._14 * pM2._42;
    pOut._13 = pM1._11 * pM2._13 + pM1._12 * pM2._23 + pM1._13 * pM2._33 + pM1._14 * pM2._43;
    pOut._14 = pM1._11 * pM2._14 + pM1._12 * pM2._24 + pM1._13 * pM2._34 + pM1._14 * pM2._44;
    pOut._21 = pM1._21 * pM2._11 + pM1._22 * pM2._21 + pM1._23 * pM2._31 + pM1._24 * pM2._41;
    pOut._22 = pM1._21 * pM2._12 + pM1._22 * pM2._22 + pM1._23 * pM2._32 + pM1._24 * pM2._42;
    pOut._23 = pM1._21 * pM2._13 + pM1._22 * pM2._23 + pM1._23 * pM2._33 + pM1._24 * pM2._43;
    pOut._24 = pM1._21 * pM2._14 + pM1._22 * pM2._24 + pM1._23 * pM2._34 + pM1._24 * pM2._44;
    pOut._31 = pM1._31 * pM2._11 + pM1._32 * pM2._21 + pM1._33 * pM2._31 + pM1._34 * pM2._41;
    pOut._32 = pM1._31 * pM2._12 + pM1._32 * pM2._22 + pM1._33 * pM2._32 + pM1._34 * pM2._42;
    pOut._33 = pM1._31 * pM2._13 + pM1._32 * pM2._23 + pM1._33 * pM2._33 + pM1._34 * pM2._43;
    pOut._34 = pM1._31 * pM2._14 + pM1._32 * pM2._24 + pM1._33 * pM2._34 + pM1._34 * pM2._44;
    pOut._41 = pM1._41 * pM2._11 + pM1._42 * pM2._21 + pM1._43 * pM2._31 + pM1._44 * pM2._41;
    pOut._42 = pM1._41 * pM2._12 + pM1._42 * pM2._22 + pM1._43 * pM2._32 + pM1._44 * pM2._42;
    pOut._43 = pM1._41 * pM2._13 + pM1._42 * pM2._23 + pM1._43 * pM2._33 + pM1._44 * pM2._43;
    pOut._44 = pM1._41 * pM2._14 + pM1._42 * pM2._24 + pM1._43 * pM2._34 + pM1._44 * pM2._44;
    return pOut;
}

inline D3DMATRIX CreateMatrix(Vector3 rot, Vector3 origin) {
    const float DEG_TO_RAD = static_cast<float>(3.14159265358979323846) / 180.f;
    const float radPitch = rot.x * DEG_TO_RAD;
    const float radYaw = rot.y * DEG_TO_RAD;
    const float radRoll = rot.z * DEG_TO_RAD;

    const float SP = sinf(radPitch);
    const float CP = cosf(radPitch);
    const float SY = sinf(radYaw);
    const float CY = cosf(radYaw);
    const float SR = sinf(radRoll);
    const float CR = cosf(radRoll);

    D3DMATRIX matrix;
    matrix.m[0][0] = CP * CY;
    matrix.m[0][1] = CP * SY;
    matrix.m[0][2] = SP;
    matrix.m[0][3] = 0.f;

    matrix.m[1][0] = SR * SP * CY - CR * SY;
    matrix.m[1][1] = SR * SP * SY + CR * CY;
    matrix.m[1][2] = -SR * CP;
    matrix.m[1][3] = 0.f;

    matrix.m[2][0] = -(CR * SP * CY + SR * SY);
    matrix.m[2][1] = CY * SR - CR * SP * SY;
    matrix.m[2][2] = CR * CP;
    matrix.m[2][3] = 0.f;

    matrix.m[3][0] = origin.x;
    matrix.m[3][1] = origin.y;
    matrix.m[3][2] = origin.z;
    matrix.m[3][3] = 1.f;

    return matrix;
}

inline std::string GetNameById(uint32_t actor_id) {
    char pNameBuffer[256] = { 0 };
    int TableLocation = (unsigned int)(actor_id >> 0x10);
    uint16_t RowLocation = (unsigned __int16)actor_id;
    uint64_t GNameTable = Kernel->g_process_base + offsets::GNames;
    uint64_t TableLocationAddress = Kernel->read<uint64_t>(GNameTable + 0x10 + TableLocation * 0x8) + (unsigned __int32)(4 * RowLocation);
    uint64_t sLength = (unsigned __int64)(Kernel->read<uint16_t>(TableLocationAddress + 4)) >> 1;

    if (sLength > 0 && sLength < 128) {
        if (Kernel->read_physical(reinterpret_cast<PVOID>(TableLocationAddress + 6), pNameBuffer, static_cast<DWORD>(sLength))) {
            return std::string(pNameBuffer);
        }
    }
    return std::string("NULL");
}
extern std::vector<EntityList> entityList;