class Failure
    letsFailWithStack: ->
        console.log((new Error()).stack)

window.failure = () ->
    failure = new Failure
    failure.letsFailWithStack()
