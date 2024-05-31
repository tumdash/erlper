erlper
=====

An OTP application

Build
-----

    $ ./rebar3 compile

Play in shell
-----

    $ ./rebar3 shell
    1> erlper:start("erlper_drv").
    <0.204.0>
    2> erlper:foo(5).
    6
    3> erlper:bar(5).
    25
    4> 
