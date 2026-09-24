import QtQuick 2.12

Item {
    width: 640
    height: 400
    property bool transformAncestor: false
    property string mode: "scale"

    Item {
        objectName: "backgroundTitle"
        x: 20; y: 20; width: 600; height: 360
    }
    Item {
        id: ancestor
        x: 300; y: 160
        width: 100; height: 40
        transformOrigin: Item.TopLeft
        scale: transformAncestor ? (mode === "scale" ? 2 : mode === "shrink" ? 0.5 : mode === "zero" ? 0 : 1) : 1
        rotation: transformAncestor && mode === "rotation" ? 45 : 0
        transform: Scale {
            xScale: transformAncestor && mode === "matrix" ? 2 : 1
            yScale: transformAncestor && mode === "matrix" ? 0.5 : 1
        }

        Rectangle {
            objectName: "target"
            width: 100; height: 40
            color: "steelblue"
            transformOrigin: Item.TopLeft
            scale: !transformAncestor ? (mode === "scale" ? 2 : mode === "shrink" ? 0.5 : mode === "zero" ? 0 : 1) : 1
            rotation: !transformAncestor && mode === "rotation" ? 45 : 0
            transform: Scale {
                xScale: !transformAncestor && mode === "matrix" ? 2 : 1
                yScale: !transformAncestor && mode === "matrix" ? 0.5 : 1
            }
        }
    }
}
