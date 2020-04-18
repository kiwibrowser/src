This directory contains helper classes to embed a `Window`.

## `RemoteViewProvider`

`RemoteViewProvider` wraps the work needed to provide a `Window` to another
(remote) client. Typical usage is to create an instance of it with the window to
provide to the remote client.

e.g.
```
  ...

  // Step 1: Prepares for embedding after |window_for_remote_client_| is ready.
  void StartEmbed() {
    remote_view_provider_ =
        std::make_unique<views::RemoteViewProvider>(window_for_remote_client_);
    remote_view_client->GetEmbedToken(base::BindOnce(
        &EmbeddedClient::OnGotEmbedToken, base::Unretained(this)));
  }

  void OnGotEmbedToken(const base::UnguessableToken& token) {
    // Step 2: Pass |token| to the embedder.
  }

  // A Window to to provide to the remote client.
  aura::Window* window_for_remote_client_;

  // Helper to prepare |window_for_remote_client_| for embedding.
  std::unique_ptr<views::RemoteViewProvider> remote_view_provider_;

  ...
```

## `RemoteViewHost`

`RemoteViewHost` wraps the work on the embedder side and is a `NativeViewHost`
that embeds the window from the `RemoteViewProvider`. `RemoveViewHost` is a
`View` that you add to your existing `View` hierarchy.

e.g.
```
  class EmbedderView : public views::View {
   public:
    ...
    // Creates a RemoteViewHost using |token| from the embedded client.
    void AddEmbeddedView(const base::UnguessableToken& token) {
      // Ownership will be passed to the view hierarchy in AddChildView.
      views::RemoteViewHost* remote_view_host = new RemoteViewHost();
      remote_view_host->EmbedUsingToken(token, base::DoNothing());
      AddChildView(remote_view_host);
    }
    ...
  };
```

