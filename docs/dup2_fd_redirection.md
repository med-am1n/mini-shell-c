# dup2 and File Descriptor Redirection

## Standard File Descriptors

| FD | Name         | Default Target |
|----|--------------|----------------|
| 0  | STDIN_FILENO | Keyboard       |
| 1  | STDOUT_FILENO| Terminal       |
| 2  | STDERR_FILENO| Terminal       |

---

## File Descriptor Table

Each process has its **own** FD table — independent from all other processes.

An FD is just an **integer index** into that table. Each entry points to a kernel **open-file description** (which tracks the file, offset, flags, etc.).

```
Process A          Process B
FD table           FD table
0 -> keyboard      0 -> keyboard
1 -> terminal      1 -> some_file
2 -> terminal      2 -> terminal
```

Same integer (e.g. `1`) in two different processes can point to entirely different things.

---

## open()

```c
int fd = open("out.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
// fd = 3
```

FD table after `open()`:
```
0 -> keyboard
1 -> terminal
2 -> terminal
3 -> out.txt
```

## dup2()

```c
dup2(fd, STDOUT_FILENO);  // dup2(3, 1)
```

FD table after `dup2()`:
```
0 -> keyboard
1 -> out.txt      ← remapped
2 -> terminal
3 -> out.txt
```

`STDOUT_FILENO` now points to the same kernel open-file object as `fd`.

## close()

```c
close(fd);  // close(3)
```

FD table after `close()`:
```
0 -> keyboard
1 -> out.txt      ← still works
2 -> terminal
```

---

## Pipe Example

```c
int fd[2];
pipe(fd);
// fd[0] = 3  (read end)
// fd[1] = 4  (write end)
```

FD table after `pipe()`:
```
0 -> keyboard
1 -> terminal
2 -> terminal
3 -> pipe read end
4 -> pipe write end
```

### Redirect stdout → pipe write end

```c
dup2(fd[1], STDOUT_FILENO);  // dup2(4, 1)
```

```
0 -> keyboard
1 -> pipe write end   ← remapped
2 -> terminal
3 -> pipe read end
4 -> pipe write end
```

### Redirect stdin ← pipe read end

```c
dup2(fd[0], STDIN_FILENO);  // dup2(3, 0)
```

```
0 -> pipe read end    ← remapped
1 -> terminal
2 -> terminal
3 -> pipe read end
4 -> pipe write end
```

---

## Summary

| Call | Effect |
|------|--------|
| `pipe(fd)` | Creates a kernel pipe; returns two FDs (read + write ends) |
| `dup2(oldfd, newfd)` | Remaps `newfd` to the same open-file object as `oldfd` |
| `close(fd)` | Removes that FD from the table; underlying file stays open if other FDs reference it |

Redirection = changing the FD table. Pipes connect one process's stdout to another's stdin.