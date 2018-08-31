#include "Columns.h"
#include <math.h>
#include <QDebug>

Columns::Columns()
{
    initColumns();
}

bool
Columns::inRange(QString column, Vec3f soma, QVector<float> range)
{
    if(column.contains("Septum")){
        column = column.split("_")[2];
    }
    if (mBarrels.find(column) == mBarrels.end())
    {
        return false;
    }

    Barrel b = mBarrels[column];
    Vec3f center(b.center_x, b.center_y, b.center_z);
    Vec3f u = center - soma;
    float relCenter = u[0] * b.n_x + u[1] * b.n_y + u[2] * b.n_z;
    float centerPia = b.top + 0.5f * (b.bottom - b.top);
    float somaPia = centerPia + relCenter;
    //qDebug() << somaPia;
    return somaPia >= range[0] && somaPia <= range[1];
}

void
Columns::initColumns()
{
    mBarrels["A1"] = Barrel(-482.792f, 1146.67f, 8.05248f, 0.0536427f, 0.19139f, 0.980047f, 164.971f, 455, 777);
    mBarrels["A2"] = Barrel(-150.439f, 1159.39f, -46.9216f, 0.123246f, 0.277435f, 0.952806f, 162.541f, 467, 805);
    mBarrels["A3"] = Barrel(121.05f, 1136.53f, -90.7421f, 0.162841f, 0.306681f, 0.937779f, 143.619f, 485, 800);
    mBarrels["A4"] = Barrel(370.245f, 1068.77f, -129.452f, 0.195667f, 0.324575f, 0.9254f, 143.841f, 489, 805);
    mBarrels["Alpha"] = Barrel(-755.255f, 972.717f, 21.974f, 0.00538765f, 0.153553f, 0.988126f, 167.271f, 479, 788);
    mBarrels["B1"] = Barrel(-503.922f, 749.452f, 34.084f, 0.00791207f, 0.12081f, 0.992644f, 166.794f, 481, 827);
    mBarrels["B2"] = Barrel(-144.732f, 784.712f, -4.9134f, 0.0904869f, 0.174858f, 0.980427f, 169.351f, 490, 834);
    mBarrels["B3"] = Barrel(189.255f, 775.615f, -55.5768f, 0.134744f, 0.224254f, 0.965171f, 156.149f, 490, 844);
    mBarrels["B4"] = Barrel(492.225f, 782.398f, -105.589f, 0.17115f, 0.274337f, 0.946281f, 158.476f, 501, 853);
    mBarrels["Beta"] = Barrel(-885.283f, 603.416f, 38.9095f, -0.066383f, 0.0589958f, 0.996049f, 180.188f, 472, 810);
    mBarrels["C1"] = Barrel(-512.055f, 407.254f, 45.468f, -0.0493628f, 0.0299881f, 0.998331f, 179.302f, 478, 836);
    mBarrels["C2"] = Barrel(-87.1775f, 409.522f, 20.2263f, 0.0365085f, 0.074742f, 0.996534f, 181.069f, 496, 856);
    mBarrels["C3"] = Barrel(347.551f, 401.542f, -55.1051f, 0.110229f, 0.175431f, 0.978301f, 181.946f, 534, 889);
    mBarrels["C4"] = Barrel(753.731f, 415.997f, -125.665f, 0.136356f, 0.209222f, 0.968315f, 169.539f, 557, 920);
    mBarrels["Gamma"] = Barrel(-841.85f, 189.597f, 21.1325f, -0.111052f, -0.0285917f, 0.993403f, 201.061f, 478, 833);
    mBarrels["D1"] = Barrel(-427, 27.3315f, 26.6072f, -0.0545414f, -0.0220467f, 0.998268f, 188.814f, 491, 863);
    mBarrels["D2"] = Barrel(0, 0, 0, 1.31517e-09f, 1.56807e-09f, 1, 198.672f, 526, 888);
    mBarrels["D3"] = Barrel(415.574f, -2.85455f, -47.0356f, 0.0581512f, 0.0571241f, 0.996672f, 192.156f, 552, 920);
    mBarrels["D4"] = Barrel(834.671f, 10.8608f, -80.889f, 0.102741f, 0.118958f, 0.987569f, 181.069f, 556, 919);
    mBarrels["Delta"] = Barrel(-602.767f, -251.788f, -7.86675f, -0.104325f, -0.051254f, 0.993222f, 213.35f, 506, 860);
    mBarrels["E1"] = Barrel(-161.326f, -442.834f, -33.1163f, -0.0542488f, -0.0751003f, 0.995699f, 215.577f, 546, 888);
    mBarrels["E2"] = Barrel(273.711f, -492.549f, -51.1474f, 0.00335659f, -0.052817f, 0.998599f, 224.969f, 557, 912);
    mBarrels["E3"] = Barrel(719.237f, -485.896f, -57.1888f, 0.0326645f, -0.0213883f, 0.999237f, 224.261f, 549, 912);
    mBarrels["E4"] = Barrel(1125.34f, -371.1f, -86.3545f, 0.0930072f, 0.0360938f, 0.995011f, 197.869f, 580, 924);
}
