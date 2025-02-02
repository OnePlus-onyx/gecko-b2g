/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim: set ts=8 sts=2 et sw=2 tw=80:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef vm_ShapeProperty_h
#define vm_ShapeProperty_h

#include "mozilla/Assertions.h"

#include <stdint.h>

#include "jstypes.h"
#include "NamespaceImports.h"

#include "gc/Tracer.h"
#include "js/PropertyDescriptor.h"
#include "util/EnumFlags.h"

namespace js {

/* Limit on the number of slotful properties in an object. */
static constexpr uint32_t SHAPE_INVALID_SLOT = Bit(24) - 1;
static constexpr uint32_t SHAPE_MAXIMUM_SLOT = Bit(24) - 2;

// Flags associated with each property stored in the shape tree.
enum class ShapePropertyFlag : uint8_t {
  // Property attributes. See also JS::PropertyAttribute.
  Configurable = 1 << 0,
  Enumerable = 1 << 1,
  Writable = 1 << 2,

  // Whether this is an accessor property. Accessor properties have a slot that
  // stores a GetterSetter instance.
  AccessorProperty = 1 << 3,

  // If set, this is a custom data property. The property is exposed as a data
  // property to JS code and PropertyDescriptor, but instead of an object slot
  // it uses custom get/set logic.
  //
  // This is used to implement the special array.length and ArgumentsObject
  // properties.
  //
  // This flag is deprecated (we don't want to add more uses).
  CustomDataProperty = 1 << 4,
};

class ShapePropertyFlags : public EnumFlags<ShapePropertyFlag> {
  using Base = EnumFlags<ShapePropertyFlag>;
  using Base::Base;

 public:
  static const ShapePropertyFlags defaultDataPropFlags;

  static ShapePropertyFlags fromRaw(uint8_t flags) {
    return ShapePropertyFlags(flags);
  }

  bool configurable() const { return hasFlag(ShapePropertyFlag::Configurable); }
  bool enumerable() const { return hasFlag(ShapePropertyFlag::Enumerable); }
  bool writable() const {
    MOZ_ASSERT(isDataDescriptor());
    return hasFlag(ShapePropertyFlag::Writable);
  }

  // Note: this returns true only for plain data properties with a slot. Returns
  // false for custom data properties. See CustomDataProperty flag.
  bool isDataProperty() const {
    return !isAccessorProperty() && !isCustomDataProperty();
  }
  bool isAccessorProperty() const {
    return hasFlag(ShapePropertyFlag::AccessorProperty);
  }
  bool isCustomDataProperty() const {
    return hasFlag(ShapePropertyFlag::CustomDataProperty);
  }

  // Note: unlike isDataProperty, this returns true also for custom data
  // properties.
  bool isDataDescriptor() const { return !isAccessorProperty(); }
};

constexpr ShapePropertyFlags ShapePropertyFlags::defaultDataPropFlags = {
    ShapePropertyFlag::Configurable, ShapePropertyFlag::Enumerable,
    ShapePropertyFlag::Writable};

// ShapeProperty contains information (ShapePropertyFlags, slot number) for a
// property stored in the Shape tree. Property lookups on NativeObjects return a
// ShapeProperty.
class ShapeProperty {
  static constexpr uint32_t FlagsMask = 0xff;
  static constexpr uint32_t SlotShift = 8;

  uint32_t slotAndFlags_ = 0;

  static_assert(SHAPE_INVALID_SLOT <= (UINT32_MAX >> SlotShift),
                "SHAPE_INVALID_SLOT must fit in slotAndFlags_");
  static_assert(SHAPE_MAXIMUM_SLOT <= (UINT32_MAX >> SlotShift),
                "SHAPE_MAXIMUM_SLOT must fit in slotAndFlags_");

 public:
  ShapeProperty(ShapePropertyFlags flags, uint32_t slot)
      : slotAndFlags_((slot << SlotShift) | flags.toRaw()) {
    MOZ_ASSERT(maybeSlot() == slot);
    MOZ_ASSERT(this->flags() == flags);
  }

  ShapeProperty(const ShapeProperty& other) = default;

  bool isDataProperty() const { return flags().isDataProperty(); }
  bool isCustomDataProperty() const { return flags().isCustomDataProperty(); }
  bool isAccessorProperty() const { return flags().isAccessorProperty(); }
  bool isDataDescriptor() const { return flags().isDataDescriptor(); }

  bool hasSlot() const { return !isCustomDataProperty(); }

  uint32_t slot() const {
    MOZ_ASSERT(hasSlot());
    MOZ_ASSERT(maybeSlot() < SHAPE_INVALID_SLOT);
    return maybeSlot();
  }

  uint32_t maybeSlot() const { return slotAndFlags_ >> SlotShift; }

  ShapePropertyFlags flags() const {
    return ShapePropertyFlags::fromRaw(slotAndFlags_ & FlagsMask);
  }
  bool writable() const { return flags().writable(); }
  bool configurable() const { return flags().configurable(); }
  bool enumerable() const { return flags().enumerable(); }

  JS::PropertyAttributes propAttributes() const {
    JS::PropertyAttributes attrs{};
    if (configurable()) {
      attrs += JS::PropertyAttribute::Configurable;
    }
    if (enumerable()) {
      attrs += JS::PropertyAttribute::Enumerable;
    }
    if (isDataDescriptor() && writable()) {
      attrs += JS::PropertyAttribute::Writable;
    }
    return attrs;
  }

  bool operator==(const ShapeProperty& other) const {
    return slotAndFlags_ == other.slotAndFlags_;
  }
  bool operator!=(const ShapeProperty& other) const {
    return !operator==(other);
  }
};

class ShapePropertyWithKey : public ShapeProperty {
  PropertyKey key_;

 public:
  ShapePropertyWithKey(ShapePropertyFlags flags, uint32_t slot, PropertyKey key)
      : ShapeProperty(flags, slot), key_(key) {}

  PropertyKey key() const { return key_; }

  void trace(JSTracer* trc) {
    TraceRoot(trc, &key_, "ShapePropertyWithKey-key");
  }
};

template <class Wrapper>
class WrappedPtrOperations<ShapePropertyWithKey, Wrapper> {
  const ShapePropertyWithKey& value() const {
    return static_cast<const Wrapper*>(this)->get();
  }

 public:
  bool isDataProperty() const { return value().isDataProperty(); }
  uint32_t slot() const { return value().slot(); }
  PropertyKey key() const { return value().key(); }
  ShapePropertyFlags flags() const { return value().flags(); }
};

}  // namespace js

#endif /* vm_ShapeProperty_h */
