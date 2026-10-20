# NativeUfs

Native UFS storage driver. Owns the host controller, SMMU, queues and the root read path.

Current state: provides the native UFS root on sheng.

Linked into XNU by `unixnu build kernel`.
