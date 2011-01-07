
#include <vector>

using namespace std;

class GeoPoint {
    GeoPoint();
    ~GeoPoint();
    int type;
    vector<int> connections;
    double coord[2];
};
