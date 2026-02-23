#include "runtime_config.h"

QMap<QString, QVariant> RuntimeConfig::sValues;

QVariant& RuntimeConfig::value( const QString& key )
{ return sValues[ key ]; }

void RuntimeConfig::setValue( const QString& key, const QVariant& value )
{ sValues[key] = value; }
