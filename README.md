# MouseAPI

API mod for handling mouse events, such as mouse move, right click, and more. Should be backwards-compatible with the old touch system, though may cause discrepancies in some cases.

##

This API also provides the `Tooltip` and `ContextMenu` classes, which are exposed through attributes, making them easily usable for any mod without needing to link to this dependency:

```cpp
node->setAttribute("hjfod.mouse-api/tooltip", "This text is shown when the node is hovered!");
```
