#ifndef COLUMNS_H
#define COLUMNS_H

#include <QString>
#include <map>
#include "CIS3DVec3.h"
#include <QString>
#include <QVector>

class Columns
{
public:
    Columns();

    bool inRange(QString column, Vec3f soma, QVector<float> range);

private:
    void initColumns();

    struct Barrel
    {
        float center_x;
        float center_y;
        float center_z;
        float n_x;
        float n_y;
        float n_z;
        float radius;
        float top;
        float bottom;

        Barrel(){}
        Barrel(float center_x,
               float center_y,
               float center_z,
               float n_x,
               float n_y,
               float n_z,
               float radius,
               float top,
               float bottom) :
            center_x(center_x),
            center_y(center_y),
            center_z(center_z),
            n_x(n_x),
            n_y(n_y),
            n_z(n_z),
            radius(radius),
            top(top),
            bottom(bottom)
        {
        }
    };

    std::map<QString, Barrel> mBarrels;
};

#endif // COLUMNS_H
