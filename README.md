
The easiest way to use villoc is probably to run the following command and open
out.html in a browser.

```shell
ltrace ./target |& villoc.py - out.html;
```

It is probably a good idea to disable ASLR for repeatable results and to use a
file to pass the ltrace to villoc because otherwise the target's error output
will be interleaved and might confuse villoc sometimes.

```shell
setarch x86_64 -R ltrace -o trace ./target; villoc.py trace out.html;
```

![image](https://pbs.twimg.com/media/CDCQCzyWYAAck7k.png:large)
