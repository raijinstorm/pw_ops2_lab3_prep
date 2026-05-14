# `getline()`

## Purpose
Read one whole line from a stream, growing the buffer if needed.

## Prototype
```c
ssize_t getline(char **lineptr, size_t *n, FILE *stream);
```

## Parameters
- `lineptr`: pointer to a buffer pointer.
- `n`: current buffer size.
- `stream`: usually `stdin`.

## Returns
- Number of bytes read, including newline if present.
- `-1` on EOF or error.

## Used In These Labs
- `task4/client.c`
- `polish_lab/sop-werona.c`

## Example
```c
char *line = NULL;
size_t size = 0;
ssize_t n = getline(&line, &size, stdin);
```

## Common Mistakes
- Forgetting to `free(line)`.
- Assuming the newline was removed for you.

## Related
- `read()`
