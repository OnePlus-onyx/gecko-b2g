/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

var EXPORTED_SYMBOLS = ["InteractionsParent"];

ChromeUtils.defineModuleGetter(
  this,
  "Interactions",
  "resource:///modules/Interactions.jsm"
);

/**
 * Receives messages from InteractionsChild and passes them to the appropriate
 * interactions object.
 */
class InteractionsParent extends JSWindowActorParent {
  receiveMessage(msg) {
    if (msg.name == "Interactions:PageLoaded") {
      Interactions.registerNewInteraction(
        this.browsingContext.embedderElement,
        msg.data
      );
    }
  }
}
