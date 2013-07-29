/*
 * Copyright 2013 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Lars Uebernickel <lars.uebernickel@canonical.com>
 */

#include "qgsettings.h"

#include "qconftypes.h"
#include "util.h"
#include <gio/gio.h>

struct QGSettingsPrivate
{
    QByteArray schema_id;
    QByteArray path;
    GSettings *settings;

    static void settingChanged(GSettings *settings, const gchar *key, gpointer user_data);
};

void QGSettingsPrivate::settingChanged(GSettings *, const gchar *key, gpointer user_data)
{
    QGSettings *self = (QGSettings *)user_data;

    Q_EMIT(self->changed(qtify_name(key)));
}

QGSettings::QGSettings(const QByteArray &schema_id, const QByteArray &path, QObject *parent):
    QObject(parent)
{
    priv = new QGSettingsPrivate;
    priv->schema_id = schema_id;
    priv->path = path;

    if (priv->path.isEmpty())
        priv->settings = g_settings_new(priv->schema_id.constData());
    else
        priv->settings = g_settings_new_with_path(priv->schema_id.constData(), priv->path.constData());

    g_signal_connect(priv->settings, "changed", G_CALLBACK(QGSettingsPrivate::settingChanged), this);
}

QGSettings::~QGSettings()
{
    g_object_unref (priv->settings);
    delete priv;
}

QVariant QGSettings::get(const QString &key) const
{
    gchar *gkey = unqtify_name(key);
    GVariant *value = g_settings_get_value(priv->settings, gkey);
    QVariant qvalue = qconf_types_to_qvariant(value);
    g_variant_unref(value);
    g_free(gkey);
    return qvalue;
}

void QGSettings::set(const QString &key, const QVariant &value)
{
    gchar *gkey = unqtify_name(key);

    /* fetch current value to find out the exact type */
    GVariant *cur = g_settings_get_value(priv->settings, gkey);

    GVariant *new_value = qconf_types_collect_from_variant(g_variant_get_type (cur), value);
    if (new_value)
        g_settings_set_value(priv->settings, gkey, new_value);

    g_free(gkey);
    g_variant_unref (cur);
}

QStringList QGSettings::keys() const
{
    QStringList list;
    gchar **keys = g_settings_list_keys(priv->settings);
    for (int i = 0; keys[i]; i++)
        list.append(qtify_name(keys[i]));

    g_strfreev(keys);
    return list;
}

QVariantList QGSettings::choices(const QString &qkey) const
{
    gchar *key = unqtify_name (qkey);
    GVariant *range = g_settings_get_range(priv->settings, key);
    g_free(key);

    if (range == NULL)
        return QVariantList();

    const gchar *type;
    GVariant *value;
    g_variant_get(range, "(&sv)", &type, &value);

    QVariantList choices;
    if (g_str_equal(type, "enum")) {
        GVariantIter iter;
        GVariant *child;

        g_variant_iter_init (&iter, value);
        while ((child = g_variant_iter_next_value(&iter))) {
            choices.append(qconf_types_to_qvariant(child));
            g_variant_unref(child);
        }
    }

    g_variant_unref (value);
    g_variant_unref (range);

    return choices;
}