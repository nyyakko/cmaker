# 02 - Project Types

You may also specify the project type when creating it. Currently, two\
types are supported.

## 02.1 - Executable Project

> [!NOTE]
> This is the default project type.

```bash
cmaker -n my_project executable
```

## 02.2 - Library Project

To create the simplest library project, you should do as follows

```bash
cmaker -n my_library library
```

which will create a ``static`` library project.

> [!NOTE]
> To change the library kind, you have to pass the ``-k`` flag with one of the\
> following values: ``static``, ``shared`` ``header-only``.

