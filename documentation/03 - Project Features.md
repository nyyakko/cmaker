# 03 - Project Features

Projects can also have features. Currently, two features available: ``installable``, ``testable``

> [!IMPORTANT]
> Some templates have required features. They are always installed, whatever explicitly requested or not.

To make an ``installable`` and ``testable`` library project, do as follows:

```bash
cmaker -n my_library library --features installable, testable
```

> [!NOTE]
> Library projects have the feature ``installable`` as a required feature.
