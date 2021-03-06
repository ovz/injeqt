/*
 * %injeqt copyright begin%
 * Copyright 2014 Rafał Malinowski (rafal.przemyslaw.malinowski@gmail.com)
 * %injeqt copyright end%
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "default-constructor-method.h"

#include <cassert>

namespace injeqt { namespace internal {

default_constructor_method::default_constructor_method()
{
}

default_constructor_method::default_constructor_method(QMetaMethod meta_method) :
	_object_type{meta_method.enclosingMetaObject()},
	_meta_method{std::move(meta_method)}
{
	assert(meta_method.methodType() == QMetaMethod::Constructor);
	assert(meta_method.parameterCount() == 0);
	assert(meta_method.enclosingMetaObject() != nullptr);
}

bool default_constructor_method::is_empty() const
{
	return !_meta_method.isValid();
}

const type & default_constructor_method::object_type() const
{
	return _object_type;
}

const QMetaMethod & default_constructor_method::meta_method() const
{
	return _meta_method;
}

std::unique_ptr<QObject> default_constructor_method::invoke() const
{
	assert(!is_empty());

	return std::unique_ptr<QObject>{_meta_method.enclosingMetaObject()->newInstance()};
}

bool operator == (const default_constructor_method &x, const default_constructor_method &y)
{
	if (x.object_type() != y.object_type())
		return false;

	assert(x.meta_method() == y.meta_method());

	return true;
}

bool operator != (const default_constructor_method &x, const default_constructor_method &y)
{
	return !(x == y);
}

default_constructor_method make_default_constructor_method(const type &t)
{
	assert(!t.is_empty());
	assert(!t.is_qobject());

	auto meta_object = t.meta_object();
	auto constructor_count = meta_object->constructorCount();
	for (decltype(constructor_count) i = 0; i < constructor_count; i++)
	{
		auto constructor = meta_object->constructor(i);
		if (constructor.methodType() != QMetaMethod::Constructor)
			continue;
		if (constructor.parameterCount() != 0)
			continue;
		return default_constructor_method{constructor};
	}

	return default_constructor_method{};
}

}}
