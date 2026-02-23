#ifndef RUNTIME_CONFIG_H
#define RUNTIME_CONFIG_H

#include <QMap>
#include <QVariant>

class RuntimeConfig
{
    public:
        static QVariant& value( const QString& key );
        static void setValue( const QString& key, const QVariant& value );

    private:
        static QMap<QString, QVariant> sValues;
};

#endif // RUNTIME_CONFIG_H
