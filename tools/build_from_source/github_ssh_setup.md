## Connect to GitHub with SSH

with SSH keys, you can connect to GitHub and access repositories without password authentication

SSH key is important for Falcon because the git-modules have several private repositories, and you need to set up SSH authentication in order to access the private third-party repositories (libhcs and MP-SPDZ)

for example:
```bash
# without the ssh key
$ git clone git@github.com:lemonviv/libhcs.git
Cloning into 'libhcs'...
git@github.com: Permission denied (publickey).
fatal: Could not read from remote repository.

Please make sure you have the correct access rights
and the repository exists.

# with the ssh key
$ ssh-agent bash -c 'ssh-add ~/.ssh/id_rsa_forGithub; git clone git@github.com:lemonviv/libhcs.git'
Identity added: /home/svd/.ssh/id_rsa_forGithub (/home/svd/.ssh/id_rsa_forGithub)
Cloning into 'libhcs'...
remote: Enumerating objects: 1053, done.
remote: Total 1053 (delta 0), reused 0 (delta 0), pack-reused 1053
Receiving objects: 100% (1053/1053), 689.14 KiB | 1.29 MiB/s, done.
Resolving deltas: 100% (652/652), done.
```

the dockerfile and other installation scripts assume you are using SSH (instead of HTTPS) for remote URLs to the needed repos

### Checking for existing SSH keys

```bash
$ ls -al ~/.ssh
# Lists the files in your .ssh directory, if they exist
```

public ssh key-pairs should be:
```
id_ed25519.pub
id_rsa.pub
```

### Generate new SSH key

Use your GitHub email address.

for **ed25519**:
```bash
$ ssh-keygen -t ed25519 -C "your_email@example.com"
```

for **rsa**:
```bash
$ ssh-keygen -t rsa -b 4096 -C "your_email@example.com"
```

### Add the SSH key to ssh-agent

Start the ssh-agent in the background.
```bash
$ eval "$(ssh-agent -s)"
> Agent pid 59566
```

Add your SSH private key to the ssh-agent with `ssh-add`

### Adding a new SSH key to your GitHub account

**NOTE: GitHub wants your PUBLIC key, not the private key**

Copy the SSH public key to your GitHub


### Testing your SSH connection to GitHub

```bash
$ ssh -T git@github.com
# Attempts to ssh to GitHub
```

#### Reference

- https://docs.github.com/en/free-pro-team@latest/github/authenticating-to-github/connecting-to-github-with-ssh


