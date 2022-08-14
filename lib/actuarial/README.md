This library simply defines some known actuarial functions like <code><sub>n</sub>E<sub>x</sub></code> and <code><sub>n</sub>a<sub>x</sub></code>.

I won't go into details about actuarial science, but if you do a google
search on annuity due and annuity immediate then you will find a ton of useful
documentation.

As a simple example, given a table "table.txt" which has the form:

0,100

1,99

...

If you would want to know what the chance of death is from 0 to 1 years old you would run:

`double chance = npx(table, 0, 1, 0);`

In this case `chance` should equal `0.99` (`99/100`).
The final `0` in the `npx` call is "age correction" which means you can shift the ages in the call. For example a call from age 40 to 41 with age correction equal to `-5` would be the same as a call from 35 to 36.
