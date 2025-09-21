# How to work with github organization

## 1. Clone the repo(If you didnt clone)

~~~
git clone git@github.com:Greenhouse-CO2-Controller/codebase.git
cd codebase
~~~

## 2. Fetch all branches

~~~
git fetch origin
~~~

## 3. Switch to your branch

~~~
git checkout your_branch_name
~~~

To see the all the branches--> git branch -a
The branch that you currently in will show with this *

## 4 Add the remote (only once)

~~~
git remote add origin git@github.com:Greenhouse-CO2-Controller/codebase.git
~~~

verify connection,

~~~
git remote -v
~~~

You will see somthing like this

~~~
origin  git@github.com:Greenhouse-CO2-Controller/codebase.git (fetch)
origin  git@github.com:Greenhouse-CO2-Controller/codebase.git (push)
~~~

## 5. Commit

Once you change the code,
~~~
gid add .
git commit -m "Commit message"
~~~

git add . (adding all files) or git add file_name1 file_name2

## 6. Push your changes to GitHub

~~~
git push origin your_branch_name
~~~

Note: Always make sure you are pushing to your branch





