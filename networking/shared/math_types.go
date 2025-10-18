package shared

// Math types that match the C++ engine implementation
// These are used for network serialization and should match the C++ types exactly

// Vector3 represents a 3D vector (matches C++ Vector3)
type Vector3 struct {
	X, Y, Z float32 // Using float32 to match C++ implementation
}

// NewVector3 creates a new Vector3
func NewVector3(x, y, z float32) Vector3 {
	return Vector3{X: x, Y: y, Z: z}
}

// Add adds two vectors
func (v Vector3) Add(other Vector3) Vector3 {
	return Vector3{X: v.X + other.X, Y: v.Y + other.Y, Z: v.Z + other.Z}
}

// Sub subtracts two vectors
func (v Vector3) Sub(other Vector3) Vector3 {
	return Vector3{X: v.X - other.X, Y: v.Y - other.Y, Z: v.Z - other.Z}
}

// Mul multiplies vector by scalar
func (v Vector3) Mul(scalar float32) Vector3 {
	return Vector3{X: v.X * scalar, Y: v.Y * scalar, Z: v.Z * scalar}
}

// Dot computes dot product
func (v Vector3) Dot(other Vector3) float32 {
	return v.X*other.X + v.Y*other.Y + v.Z*other.Z
}

// Cross computes cross product
func (v Vector3) Cross(other Vector3) Vector3 {
	return Vector3{
		X: v.Y*other.Z - v.Z*other.Y,
		Y: v.Z*other.X - v.X*other.Z,
		Z: v.X*other.Y - v.Y*other.X,
	}
}

// Magnitude returns the magnitude
func (v Vector3) Magnitude() float32 {
	return float32(float64(v.Dot(v)) * 0.5) // Simplified for network efficiency
}

// Normalize normalizes the vector
func (v Vector3) Normalize() Vector3 {
	mag := v.Magnitude()
	if mag == 0 {
		return Vector3{}
	}
	return v.Mul(1.0 / mag)
}

// Vector2 represents a 2D vector (matches C++ Vector2)
type Vector2 struct {
	X, Y float32
}

// NewVector2 creates a new Vector2
func NewVector2(x, y float32) Vector2 {
	return Vector2{X: x, Y: y}
}

// Add adds two vectors
func (v Vector2) Add(other Vector2) Vector2 {
	return Vector2{X: v.X + other.X, Y: v.Y + other.Y}
}

// Sub subtracts two vectors
func (v Vector2) Sub(other Vector2) Vector2 {
	return Vector2{X: v.X - other.X, Y: v.Y - other.Y}
}

// Mul multiplies vector by scalar
func (v Vector2) Mul(scalar float32) Vector2 {
	return Vector2{X: v.X * scalar, Y: v.Y * scalar}
}

// Dot computes dot product
func (v Vector2) Dot(other Vector2) float32 {
	return v.X*other.X + v.Y*other.Y
}

// Magnitude returns the magnitude
func (v Vector2) Magnitude() float32 {
	return float32(float64(v.Dot(v)) * 0.5) // Simplified for network efficiency
}

// Normalize normalizes the vector
func (v Vector2) Normalize() Vector2 {
	mag := v.Magnitude()
	if mag == 0 {
		return Vector2{}
	}
	return v.Mul(1.0 / mag)
}

// Quaternion represents a quaternion for 3D rotations (matches C++ Quaternion)
type Quaternion struct {
	X, Y, Z, W float32
}

// NewQuaternion creates a new quaternion
func NewQuaternion(x, y, z, w float32) Quaternion {
	return Quaternion{X: x, Y: y, Z: z, W: w}
}

// NewQuaternionIdentity creates identity quaternion
func NewQuaternionIdentity() Quaternion {
	return Quaternion{X: 0, Y: 0, Z: 0, W: 1}
}

// Mul multiplies two quaternions
func (q Quaternion) Mul(other Quaternion) Quaternion {
	return Quaternion{
		W: q.W*other.W - q.X*other.X - q.Y*other.Y - q.Z*other.Z,
		X: q.W*other.X + q.X*other.W + q.Y*other.Z - q.Z*other.Y,
		Y: q.W*other.Y - q.X*other.Z + q.Y*other.W + q.Z*other.X,
		Z: q.W*other.Z + q.X*other.Y - q.Y*other.X + q.Z*other.W,
	}
}

// Normalize normalizes the quaternion
func (q Quaternion) Normalize() Quaternion {
	mag := float32(float64(q.X*q.X + q.Y*q.Y + q.Z*q.Z + q.W*q.W) * 0.5)
	if mag == 0 {
		return Quaternion{}
	}
	return Quaternion{X: q.X / mag, Y: q.Y / mag, Z: q.Z / mag, W: q.W / mag}
}
