# Contributing to MPC-HC

## Pull Requests

If you want to help, here's what you need to do:

1. Make sure you have a [GitHub account](https://github.com/signup/free).
2. [Fork](https://github.com/clsid2/mpc-hc/fork) our repository.
3. Create a new topic branch (based on the `develop` branch) to contain your feature, change, or fix.
4. **Set `core.autocrlf` to true: `git config core.autocrlf true`.**
5. [Open a Pull Request](https://github.com/clsid2/mpc-hc/pulls) with a clear title and description.

### General development guidelines

1. Apart from the above instructions, try to keep your patches clean **without** any unrelated changes.
2. Keep your branches in good shape; don't mix patches that do different things and **always**
   try to squash when it makes sense. For example, you made 3 patches that refer to the same thing;
   squash them into one.
3. When the resource files change, make sure you run [sync.bat](/src/mpc-hc/mpcresources/sync.bat)
   (requires Python so check [Compilation.md](/docs/Compilation.md)'s Part B) to sync the resource files
   and verify that everything went right.
4. Generally we try not to touch third-party code. If some change is needed there:
   1. Try to provide patches to the upstream project.
   2. If for any reason this takes a long time or isn't possible, make a clear patch, mark custom code as
      such.
5. Try to make sure all newly added files have a license header with copyright info and the year it was
   created/edited.
