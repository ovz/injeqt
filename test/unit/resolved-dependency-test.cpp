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

#include "exception/empty-type.cpp"
#include "exception/exception.cpp"
#include "exception/interface-not-implemented.cpp"
#include "exception/invalid-qobject.cpp"
#include "exception/qobject-type.cpp"
#include "implementation.cpp"
#include "interfaces-utils.cpp"
#include "resolved-dependency.cpp"
#include "setter-method.cpp"
#include "type.cpp"

#include "expect.h"
#include "make-setter-method.h"
#include "utils.h"

#include <QtTest/QtTest>

using namespace injeqt::internal;
using namespace injeqt::v1;

class type_1 : public QObject
{
	Q_OBJECT
};

class type_1_subtype_1 : public QObject
{
	Q_OBJECT
};

class type_2 : public QObject
{
	Q_OBJECT
};

class injected_type : public QObject
{
	Q_OBJECT

public:
	type_1 *_1 = nullptr;
	type_2 *_2 = nullptr;

public slots:
	INJEQT_SETTER void setter_1(type_1 *a) { _1 = a; }
	INJEQT_SETTER void setter_1_subtype_1(type_1_subtype_1 *) {}
	INJEQT_SETTER void setter_2(type_2 *a) { _2 = a; }

};

class resolved_dependency_test : public QObject
{
	Q_OBJECT

private slots:
	void should_properly_apply_on_valid_object();

};

void resolved_dependency_test::should_properly_apply_on_valid_object()
{
	auto object_1 = make_object<type_1>();
	auto object_2 = make_object<type_2>();
	auto apply_on_object = make_object<injected_type>();
	auto resolved_1 = resolved_dependency{implementation{make_type<type_1>(), object_1.get()}, make_setter_method<injected_type>("setter_1(type_1*)")};
	auto resolved_2 = resolved_dependency{implementation{make_type<type_2>(), object_2.get()}, make_setter_method<injected_type>("setter_2(type_2*)")};

	QCOMPARE(static_cast<QObject *>(nullptr), static_cast<injected_type *>(apply_on_object.get())->_1);
	QCOMPARE(static_cast<QObject *>(nullptr), static_cast<injected_type *>(apply_on_object.get())->_2);

	resolved_1.apply_on(apply_on_object.get());

	QCOMPARE(object_1.get(), static_cast<injected_type *>(apply_on_object.get())->_1);
	QCOMPARE(static_cast<QObject *>(nullptr), static_cast<injected_type *>(apply_on_object.get())->_2);

	resolved_2.apply_on(apply_on_object.get());

	QCOMPARE(object_1.get(), static_cast<injected_type *>(apply_on_object.get())->_1);
	QCOMPARE(object_2.get(), static_cast<injected_type *>(apply_on_object.get())->_2);
}

QTEST_APPLESS_MAIN(resolved_dependency_test)
#include "resolved-dependency-test.moc"
