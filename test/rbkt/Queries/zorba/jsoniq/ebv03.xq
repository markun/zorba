import module namespace j = "http://www.jsoniq.org/functions";

(: ebv of a sequence of pairs :)
let $j := { "foo" : [ 1 to 10 ], "bar" : true }
return fn:boolean(j:pairs($j))

