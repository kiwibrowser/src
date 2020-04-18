description("Tests that dispatchEvent's argument is required to be an Event.")

shouldThrow("document.dispatchEvent(null)");
shouldThrow("document.dispatchEvent(document)");
