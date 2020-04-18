# `Tab` lifecycle

`LegacyTabHelper` creates `Tab` and keep it retained. `AttachTabHelpers`
creates `LegacyTabHelper` first and then all the other tab helpers. The
`AttachTabHelpers` method can be invoked on a newly created `WebState`.

From that point, `LegacyTabHelper::GetTabFromWebState()` will return the
`Tab` associated with `WebState`. That method will return `nil` before
the call to `AttachTabHelpers`.

````cpp
web::WebState::CreateParams params{...};
std::unique_ptr<web::WebState> web_state =  web::WebState::Create(params);
AttachTabHelper(web_state.get());

Tab* tab = LegacyTabHelper::GetFromWebState(web_state.get());
DCHECK(tab != nil);
````

When a `WebState` is added to a `TabModel`'s `WebStateList`,
`TabModelWebStateListDelegate` will invoke `AttachTabHelpers` if necessary.

```cpp
TabModel* tab_model = ...;
std::unique_ptr<web::WebState> web_state =  ...;
[tab_model webStateList]->InsertWebState(0, std::move(web_state));
Tab* tab = LegacyTabHelper::GetFromWebState(
    [tab_model webStateList]->GetWebStateAt(0));
DCHECK(tab != nil);
```

`Tab` register itself as a `WebStateObserver`. When `-webStateDestroyed:`
is invoked as part of `WebState` destruction, `Tab` destroys its state and
should no longer be used (as `-webState` will return `nullptr`).

`LegacyTabHelper` is a `WebStateUserData` thus it is destroyed after the
`WebState` destructor completes. `LegacyTabHelper` release its reference
to `Tab` when destroyed.

It is better to only use `WebState` and to access the `Tab` via
`LegacyTabHelper` as `Tab` will be removed in the new architecture.
