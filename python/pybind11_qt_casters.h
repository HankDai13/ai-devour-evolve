#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <QList>
#include <QVector>
#include <QMap>
#include <QPointF>

namespace pybind11 { namespace detail {

// QVector<T> 转换器 (如果尚未定义)
#ifndef PYBIND11_QT_VECTOR_CASTER
#define PYBIND11_QT_VECTOR_CASTER
template <typename T>
struct type_caster<QVector<T>> {
public:
    PYBIND11_TYPE_CASTER(QVector<T>, _("List[") + make_caster<T>::name + _("]"));

    bool load(handle src, bool) {
        if (!isinstance<sequence>(src))
            return false;
        auto s = reinterpret_borrow<sequence>(src);
        value.clear();
        value.reserve(s.size());
        for (auto it : s) {
            auto item_caster = make_caster<T>();
            if (!item_caster.load(it, false))
                return false;
            value.append(cast_op<T &&>(std::move(item_caster)));
        }
        return true;
    }

    static handle cast(const QVector<T> &src, return_value_policy policy, handle parent) {
        list l(src.size());
        size_t index = 0;
        for (auto const &value : src) {
            auto value_ = reinterpret_steal<object>(make_caster<T>::cast(value, policy, parent));
            if (!value_)
                return handle();
            PyList_SET_ITEM(l.ptr(), (ssize_t) index++, value_.release().ptr());
        }
        return l.release();
    }
};
#endif

// QMap<K,V> 转换器
#ifndef PYBIND11_QT_MAP_CASTER
#define PYBIND11_QT_MAP_CASTER
template <typename K, typename V>
struct type_caster<QMap<K, V>> {
    using type = QMap<K, V>;
    PYBIND11_TYPE_CASTER(type, _("Dict[") + make_caster<K>::name + _(", ") + make_caster<V>::name + _("]"));

    bool load(handle src, bool) {
        if (!isinstance<dict>(src))
            return false;
        auto d = reinterpret_borrow<dict>(src);
        value.clear();
        for (auto it : d) {
            auto key_caster = make_caster<K>();
            auto value_caster = make_caster<V>();
            if (!key_caster.load(it.first, false) || !value_caster.load(it.second, false))
                return false;
            value.insert(cast_op<K &&>(std::move(key_caster)), cast_op<V &&>(std::move(value_caster)));
        }
        return true;
    }

    static handle cast(const QMap<K, V> &src, return_value_policy policy, handle parent) {
        dict d;
        for (auto it = src.begin(); it != src.end(); ++it) {
            auto key = reinterpret_steal<object>(make_caster<K>::cast(it.key(), policy, parent));
            auto value = reinterpret_steal<object>(make_caster<V>::cast(it.value(), policy, parent));
            if (!key || !value)
                return handle();
            d[key] = value;
        }
        return d.release();
    }
};
#endif

// QPointF 转换器
#ifndef PYBIND11_QT_POINTF_CASTER
#define PYBIND11_QT_POINTF_CASTER
template <>
struct type_caster<QPointF> {
public:
    PYBIND11_TYPE_CASTER(QPointF, _("Tuple[float, float]"));

    bool load(handle src, bool) {
        if (!isinstance<tuple>(src))
            return false;
        auto t = reinterpret_borrow<tuple>(src);
        if (t.size() != 2)
            return false;
        value.setX(t[0].cast<float>());
        value.setY(t[1].cast<float>());
        return true;
    }

    static handle cast(const QPointF &src, return_value_policy /* policy */, handle /* parent */) {
        auto tuple = pybind11::tuple(2);
        tuple[0] = src.x();
        tuple[1] = src.y();
        return tuple.release();
    }
};
#endif

}} // namespace pybind11::detail
