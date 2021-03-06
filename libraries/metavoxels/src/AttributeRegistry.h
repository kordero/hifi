//
//  AttributeRegistry.h
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__AttributeRegistry__
#define __interface__AttributeRegistry__

#include <QHash>
#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QWidget>

#include "Bitstream.h"
#include "SharedObject.h"

class QScriptContext;
class QScriptEngine;
class QScriptValue;

class Attribute;
class MetavoxelData;
class MetavoxelNode;
class MetavoxelStreamState;

typedef SharedObjectPointerTemplate<Attribute> AttributePointer;

/// Maintains information about metavoxel attribute types.
class AttributeRegistry {
public:
    
    /// Returns a pointer to the singleton registry instance.
    static AttributeRegistry* getInstance();
    
    AttributeRegistry();
    
    /// Configures the supplied script engine with the global AttributeRegistry property.
    void configureScriptEngine(QScriptEngine* engine);
    
    /// Registers an attribute with the system.  The registry assumes ownership of the object.
    /// \return either the pointer passed as an argument, if the attribute wasn't already registered, or the existing
    /// attribute
    AttributePointer registerAttribute(Attribute* attribute) { return registerAttribute(AttributePointer(attribute)); }
    
    /// Registers an attribute with the system.
    /// \return either the pointer passed as an argument, if the attribute wasn't already registered, or the existing
    /// attribute
    AttributePointer registerAttribute(AttributePointer attribute);
    
    /// Deregisters an attribute.
    void deregisterAttribute(const QString& name);
    
    /// Retrieves an attribute by name.
    AttributePointer getAttribute(const QString& name) const { return _attributes.value(name); }
    
    /// Returns a reference to the attribute hash.
    const QHash<QString, AttributePointer>& getAttributes() const { return _attributes; }
    
    /// Returns a reference to the standard SharedObjectPointer "guide" attribute.
    const AttributePointer& getGuideAttribute() const { return _guideAttribute; }
    
    /// Returns a reference to the standard SharedObjectSet "spanners" attribute.
    const AttributePointer& getSpannersAttribute() const { return _spannersAttribute; }
    
    /// Returns a reference to the standard QRgb "color" attribute.
    const AttributePointer& getColorAttribute() const { return _colorAttribute; }
    
    /// Returns a reference to the standard QRgb "normal" attribute.
    const AttributePointer& getNormalAttribute() const { return _normalAttribute; }
    
private:

    static QScriptValue getAttribute(QScriptContext* context, QScriptEngine* engine);

    QHash<QString, AttributePointer> _attributes;
    AttributePointer _guideAttribute;
    AttributePointer _spannersAttribute;
    AttributePointer _colorAttribute;
    AttributePointer _normalAttribute;
};

/// Converts a value to a void pointer.
template<class T> inline void* encodeInline(T value) {
    return *(void**)&value;
}

/// Extracts a value from a void pointer.
template<class T> inline T decodeInline(void* value) {
    return *(T*)&value;
}

/// Pairs an attribute value with its type.
class AttributeValue {
public:
    
    AttributeValue(const AttributePointer& attribute = AttributePointer());
    AttributeValue(const AttributePointer& attribute, void* value);
    
    AttributePointer getAttribute() const { return _attribute; }
    void* getValue() const { return _value; }
    
    template<class T> void setInlineValue(T value) { _value = encodeInline(value); }
    template<class T> T getInlineValue() const { return decodeInline<T>(_value); }
    
    template<class T> T* getPointerValue() const { return static_cast<T*>(_value); }
    
    void* copy() const;

    bool isDefault() const;

    bool operator==(const AttributeValue& other) const;
    bool operator==(void* other) const;
    
    bool operator!=(const AttributeValue& other) const;
    bool operator!=(void* other) const;
    
protected:
    
    AttributePointer _attribute;
    void* _value;
};

// Assumes ownership of an attribute value.
class OwnedAttributeValue : public AttributeValue {
public:
    
    /// Assumes ownership of the specified value.  It will be destroyed when this is destroyed or reassigned.
    OwnedAttributeValue(const AttributePointer& attribute, void* value);
    
    /// Creates an owned attribute with a copy of the specified attribute's default value.
    OwnedAttributeValue(const AttributePointer& attribute = AttributePointer());
    
    /// Creates an owned attribute with a copy of the specified other value.
    OwnedAttributeValue(const AttributeValue& other);
    
    /// Creates an owned attribute with a copy of the specified other value.
    OwnedAttributeValue(const OwnedAttributeValue& other);
    
    /// Destroys the current value, if any.
    ~OwnedAttributeValue();
    
    /// Destroys the current value, if any, and copies the specified other value.
    OwnedAttributeValue& operator=(const AttributeValue& other);
    
    /// Destroys the current value, if any, and copies the specified other value.
    OwnedAttributeValue& operator=(const OwnedAttributeValue& other);
};

/// Represents a registered attribute.
class Attribute : public SharedObject {
    Q_OBJECT
    Q_PROPERTY(float lodThresholdMultiplier MEMBER _lodThresholdMultiplier)
    
public:
    
    static const int MERGE_COUNT = 8;
    
    Attribute(const QString& name);
    virtual ~Attribute();

    Q_INVOKABLE QString getName() const { return objectName(); }

    float getLODThresholdMultiplier() const { return _lodThresholdMultiplier; }
    void setLODThresholdMultiplier(float multiplier) { _lodThresholdMultiplier = multiplier; }

    void* create() const { return create(getDefaultValue()); }
    virtual void* create(void* copy) const = 0;
    virtual void destroy(void* value) const = 0;

    virtual void read(Bitstream& in, void*& value, bool isLeaf) const = 0;
    virtual void write(Bitstream& out, void* value, bool isLeaf) const = 0;

    virtual void readDelta(Bitstream& in, void*& value, void* reference, bool isLeaf) const { read(in, value, isLeaf); }
    virtual void writeDelta(Bitstream& out, void* value, void* reference, bool isLeaf) const { write(out, value, isLeaf); }

    virtual void readMetavoxelRoot(MetavoxelData& data, MetavoxelStreamState& state);
    virtual void writeMetavoxelRoot(const MetavoxelNode& root, MetavoxelStreamState& state);
    
    virtual void readMetavoxelDelta(MetavoxelData& data, const MetavoxelNode& reference, MetavoxelStreamState& state);
    virtual void writeMetavoxelDelta(const MetavoxelNode& root, const MetavoxelNode& reference, MetavoxelStreamState& state);
    
    virtual void readMetavoxelSubdivision(MetavoxelData& data, MetavoxelStreamState& state);
    virtual void writeMetavoxelSubdivision(const MetavoxelNode& root, MetavoxelStreamState& state);

    virtual bool equal(void* first, void* second) const = 0;

    /// Merges the value of a parent and its children.
    /// \return whether or not the children and parent values are all equal
    virtual bool merge(void*& parent, void* children[]) const = 0;

    virtual void* getDefaultValue() const = 0;

    virtual void* createFromScript(const QScriptValue& value, QScriptEngine* engine) const { return create(); }
    
    virtual void* createFromVariant(const QVariant& value) const { return create(); }
    
    /// Creates a widget to use to edit values of this attribute, or returns NULL if the attribute isn't editable.
    /// The widget should have a single "user" property that will be used to get/set the value.
    virtual QWidget* createEditor(QWidget* parent = NULL) const { return NULL; }

private:
    
    float _lodThresholdMultiplier;
};

/// A simple attribute class that stores its values inline.
template<class T, int bits = 32> class InlineAttribute : public Attribute {
public:
    
    InlineAttribute(const QString& name, const T& defaultValue = T()) : Attribute(name), _defaultValue(defaultValue) { }
    
    virtual void* create(void* copy) const { void* value; new (&value) T(*(T*)&copy); return value; }
    virtual void destroy(void* value) const { ((T*)&value)->~T(); }
    
    virtual void read(Bitstream& in, void*& value, bool isLeaf) const;
    virtual void write(Bitstream& out, void* value, bool isLeaf) const;

    virtual bool equal(void* first, void* second) const { return decodeInline<T>(first) == decodeInline<T>(second); }

    virtual void* getDefaultValue() const { return encodeInline(_defaultValue); }

protected:
    
    T _defaultValue;
};

template<class T, int bits> inline void InlineAttribute<T, bits>::read(Bitstream& in, void*& value, bool isLeaf) const {
    if (isLeaf) {
        value = getDefaultValue();
        in.read(&value, bits);
    }
}

template<class T, int bits> inline void InlineAttribute<T, bits>::write(Bitstream& out, void* value, bool isLeaf) const {
    if (isLeaf) {
        out.write(&value, bits);
    }
}

/// Provides merging using the =, ==, += and /= operators.
template<class T, int bits = 32> class SimpleInlineAttribute : public InlineAttribute<T, bits> {
public:
    
    SimpleInlineAttribute(const QString& name, T defaultValue = T()) : InlineAttribute<T, bits>(name, defaultValue) { }
    
    virtual bool merge(void*& parent, void* children[]) const;
};

template<class T, int bits> inline bool SimpleInlineAttribute<T, bits>::merge(void*& parent, void* children[]) const {
    T& merged = *(T*)&parent;
    merged = decodeInline<T>(children[0]);
    bool allChildrenEqual = true;
    for (int i = 1; i < Attribute::MERGE_COUNT; i++) {
        merged += decodeInline<T>(children[i]);
        allChildrenEqual &= (decodeInline<T>(children[0]) == decodeInline<T>(children[i]));
    }
    merged /= Attribute::MERGE_COUNT;
    return allChildrenEqual;
}

/// Provides appropriate averaging for RGBA values.
class QRgbAttribute : public InlineAttribute<QRgb> {
    Q_OBJECT
    Q_PROPERTY(uint defaultValue MEMBER _defaultValue)

public:
    
    Q_INVOKABLE QRgbAttribute(const QString& name = QString(), QRgb defaultValue = QRgb());
    
    virtual bool merge(void*& parent, void* children[]) const;
    
    virtual void* createFromScript(const QScriptValue& value, QScriptEngine* engine) const;
    
    virtual void* createFromVariant(const QVariant& value) const;
    
    virtual QWidget* createEditor(QWidget* parent = NULL) const;
};

/// Provides appropriate averaging for packed normals.
class PackedNormalAttribute : public QRgbAttribute {
    Q_OBJECT

public:
    
    Q_INVOKABLE PackedNormalAttribute(const QString& name = QString(), QRgb defaultValue = QRgb());
    
    virtual bool merge(void*& parent, void* children[]) const;
};

/// Packs a normal into an RGB value.
QRgb packNormal(const glm::vec3& normal);

/// Unpacks a normal from an RGB value.
glm::vec3 unpackNormal(QRgb value);

/// An attribute that takes the form of QObjects of a given meta-type (a subclass of SharedObject).
class SharedObjectAttribute : public InlineAttribute<SharedObjectPointer> {
    Q_OBJECT
    Q_PROPERTY(const QMetaObject* metaObject MEMBER _metaObject)
    
public:
    
    Q_INVOKABLE SharedObjectAttribute(const QString& name = QString(),
        const QMetaObject* metaObject = &SharedObject::staticMetaObject,
        const SharedObjectPointer& defaultValue = SharedObjectPointer());

    virtual void read(Bitstream& in, void*& value, bool isLeaf) const;
    virtual void write(Bitstream& out, void* value, bool isLeaf) const;

    virtual bool merge(void*& parent, void* children[]) const;
    
    virtual void* createFromVariant(const QVariant& value) const;
    
    virtual QWidget* createEditor(QWidget* parent = NULL) const;

private:
    
    const QMetaObject* _metaObject;
};

/// An attribute that takes the form of a set of shared objects.
class SharedObjectSetAttribute : public InlineAttribute<SharedObjectSet> {
    Q_OBJECT
    Q_PROPERTY(const QMetaObject* metaObject MEMBER _metaObject)
    
public:
    
    Q_INVOKABLE SharedObjectSetAttribute(const QString& name = QString(),
        const QMetaObject* metaObject = &SharedObject::staticMetaObject);
    
    const QMetaObject* getMetaObject() const { return _metaObject; }
    
    virtual void read(Bitstream& in, void*& value, bool isLeaf) const;
    virtual void write(Bitstream& out, void* value, bool isLeaf) const;
    
    virtual bool merge(void*& parent, void* children[]) const;

    virtual QWidget* createEditor(QWidget* parent = NULL) const;

private:
    
    const QMetaObject* _metaObject;
};

/// An attribute that takes the form of a set of spanners.
class SpannerSetAttribute : public SharedObjectSetAttribute {
    Q_OBJECT

public:
    
    Q_INVOKABLE SpannerSetAttribute(const QString& name = QString(),
        const QMetaObject* metaObject = &SharedObject::staticMetaObject);
    
    virtual void readMetavoxelRoot(MetavoxelData& data, MetavoxelStreamState& state);
    virtual void writeMetavoxelRoot(const MetavoxelNode& root, MetavoxelStreamState& state);
    
    virtual void readMetavoxelDelta(MetavoxelData& data, const MetavoxelNode& reference, MetavoxelStreamState& state);
    virtual void writeMetavoxelDelta(const MetavoxelNode& root, const MetavoxelNode& reference, MetavoxelStreamState& state);
    
    virtual void readMetavoxelSubdivision(MetavoxelData& data, MetavoxelStreamState& state);
    virtual void writeMetavoxelSubdivision(const MetavoxelNode& root, MetavoxelStreamState& state);
};

#endif /* defined(__interface__AttributeRegistry__) */
