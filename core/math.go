package core

import (
	"math"
)

// Vector3 represents a 3D vector
type Vector3 struct {
	X, Y, Z float64
}

// NewVector3 creates a new Vector3
func NewVector3(x, y, z float64) *Vector3 {
	return &Vector3{X: x, Y: y, Z: z}
}

// Add adds two vectors
func (v *Vector3) Add(other *Vector3) *Vector3 {
	return &Vector3{X: v.X + other.X, Y: v.Y + other.Y, Z: v.Z + other.Z}
}

// Sub subtracts two vectors
func (v *Vector3) Sub(other *Vector3) *Vector3 {
	return &Vector3{X: v.X - other.X, Y: v.Y - other.Y, Z: v.Z - other.Z}
}

// Mul multiplies vector by scalar
func (v *Vector3) Mul(scalar float64) *Vector3 {
	return &Vector3{X: v.X * scalar, Y: v.Y * scalar, Z: v.Z * scalar}
}

// Neg negates the vector
func (v *Vector3) Neg() *Vector3 {
	return &Vector3{X: -v.X, Y: -v.Y, Z: -v.Z}
}

// Dot computes dot product
func (v *Vector3) Dot(other *Vector3) float64 {
	return v.X*other.X + v.Y*other.Y + v.Z*other.Z
}

// Cross computes cross product
func (v *Vector3) Cross(other *Vector3) *Vector3 {
	return &Vector3{
		X: v.Y*other.Z - v.Z*other.Y,
		Y: v.Z*other.X - v.X*other.Z,
		Z: v.X*other.Y - v.Y*other.X,
	}
}

// Magnitude returns the magnitude
func (v *Vector3) Magnitude() float64 {
	return math.Sqrt(v.Dot(v))
}

// MagnitudeSq returns the squared magnitude
func (v *Vector3) MagnitudeSq() float64 {
	return v.Dot(v)
}

// Normalize normalizes the vector
func (v *Vector3) Normalize() *Vector3 {
	mag := v.Magnitude()
	if mag == 0 {
		return &Vector3{}
	}
	return v.Mul(1.0 / mag)
}

// Lerp linear interpolates
func (v *Vector3) Lerp(other *Vector3, t float64) *Vector3 {
	return v.Add(other.Sub(v).Mul(t))
}

// Vector2 represents a 2D vector
type Vector2 struct {
	X, Y float64
}

// NewVector2 creates a new Vector2
func NewVector2(x, y float64) *Vector2 {
	return &Vector2{X: x, Y: y}
}

// Add adds two vectors
func (v *Vector2) Add(other *Vector2) *Vector2 {
	return &Vector2{X: v.X + other.X, Y: v.Y + other.Y}
}

// Sub subtracts two vectors
func (v *Vector2) Sub(other *Vector2) *Vector2 {
	return &Vector2{X: v.X - other.X, Y: v.Y - other.Y}
}

// Mul multiplies vector by scalar
func (v *Vector2) Mul(scalar float64) *Vector2 {
	return &Vector2{X: v.X * scalar, Y: v.Y * scalar}
}

// Dot dot product
func (v *Vector2) Dot(other *Vector2) float64 {
	return v.X*other.X + v.Y*other.Y
}

// Magnitude magnitude
func (v *Vector2) Magnitude() float64 {
	return math.Sqrt(v.Dot(v))
}

// Normalize normalize
func (v *Vector2) Normalize() *Vector2 {
	mag := v.Magnitude()
	if mag == 0 {
		return &Vector2{}
	}
	return v.Mul(1.0 / mag)
}

// Matrix4 represents a 4x4 matrix
type Matrix4 struct {
	M [16]float64
}

// NewMatrix4Identity creates an identity matrix
func NewMatrix4Identity() *Matrix4 {
	m := &Matrix4{}
	m.M[0] = 1; m.M[5] = 1; m.M[10] = 1; m.M[15] = 1
	return m
}

// Mul multiplies two matrices
func (m *Matrix4) Mul(other *Matrix4) *Matrix4 {
	result := &Matrix4{}
	for i := 0; i < 4; i++ {
		for j := 0; j < 4; j++ {
			sum := 0.0
			for k := 0; k < 4; k++ {
				sum += m.M[i*4+k] * other.M[k*4+j]
			}
			result.M[i*4+j] = sum
		}
	}
	return result
}

// Transform applies matrix to vector
func (m *Matrix4) Transform(v *Vector3) *Vector3 {
	x := m.M[0]*v.X + m.M[4]*v.Y + m.M[8]*v.Z + m.M[12]
	y := m.M[1]*v.X + m.M[5]*v.Y + m.M[9]*v.Z + m.M[13]
	z := m.M[2]*v.X + m.M[6]*v.Y + m.M[10]*v.Z + m.M[14]
	w := m.M[3]*v.X + m.M[7]*v.Y + m.M[11]*v.Z + m.M[15]
	return &Vector3{X: x / w, Y: y / w, Z: z / w}
}

// Translate creates a translation matrix
func NewMatrix4Translate(v *Vector3) *Matrix4 {
	m := NewMatrix4Identity()
	m.M[12] = v.X
	m.M[13] = v.Y
	m.M[14] = v.Z
	return m
}

// RotateX creates a rotation matrix around X axis
func NewMatrix4RotateX(angle float64) *Matrix4 {
	m := NewMatrix4Identity()
	c := math.Cos(angle)
	s := math.Sin(angle)
	m.M[5] = c
	m.M[6] = -s
	m.M[9] = s
	m.M[10] = c
	return m
}

// Scale creates a scale matrix
func NewMatrix4Scale(v *Vector3) *Matrix4 {
	m := NewMatrix4Identity()
	m.M[0] = v.X
	m.M[5] = v.Y
	m.M[10] = v.Z
	return m
}

// Quaternion represents a quaternion for 3D rotations
type Quaternion struct {
	X, Y, Z, W float64
}

// NewQuaternion creates a new quaternion
func NewQuaternion(x, y, z, w float64) *Quaternion {
	return &Quaternion{X: x, Y: y, Z: z, W: w}
}

// NewQuaternionIdentity creates identity quaternion
func NewQuaternionIdentity() *Quaternion {
	return &Quaternion{X: 0, Y: 0, Z: 0, W: 1}
}

// NewQuaternionFromAxisAngle creates quaternion from axis and angle
func NewQuaternionFromAxisAngle(axis *Vector3, angle float64) *Quaternion {
	halfAngle := angle * 0.5
	s := math.Sin(halfAngle)
	c := math.Cos(halfAngle)
	normalized := axis.Normalize()
	return &Quaternion{
		X: normalized.X * s,
		Y: normalized.Y * s,
		Z: normalized.Z * s,
		W: c,
	}
}

// Mul multiplies two quaternions
func (q *Quaternion) Mul(other *Quaternion) *Quaternion {
	return &Quaternion{
		W: q.W*other.W - q.X*other.X - q.Y*other.Y - q.Z*other.Z,
		X: q.W*other.X + q.X*other.W + q.Y*other.Z - q.Z*other.Y,
		Y: q.W*other.Y - q.X*other.Z + q.Y*other.W + q.Z*other.X,
		Z: q.W*other.Z + q.X*other.Y - q.Y*other.X + q.Z*other.W,
	}
}

// Conjugate conjugates the quaternion
func (q *Quaternion) Conjugate() *Quaternion {
	return &Quaternion{X: -q.X, Y: -q.Y, Z: -q.Z, W: q.W}
}

// Normalize normalizes the quaternion
func (q *Quaternion) Normalize() *Quaternion {
	mag := math.Sqrt(q.X*q.X + q.Y*q.Y + q.Z*q.Z + q.W*q.W)
	return &Quaternion{X: q.X / mag, Y: q.Y / mag, Z: q.Z / mag, W: q.W / mag}
}

// ToMatrix4 converts quaternion to rotation matrix
func (q *Quaternion) ToMatrix4() *Matrix4 {
	xx := q.X * q.X
	xy := q.X * q.Y
	xz := q.X * q.Z
	xw := q.X * q.W
	yy := q.Y * q.Y
	yz := q.Y * q.Z
	yw := q.Y * q.W
	zz := q.Z * q.Z
	zw := q.Z * q.W

	m := NewMatrix4Identity()
	m.M[0] = 1 - 2*(yy+zz)
	m.M[1] = 2 * (xy - zw)
	m.M[2] = 2 * (xz + yw)
	m.M[4] = 2 * (xy + zw)
	m.M[5] = 1 - 2*(xx+zz)
	m.M[6] = 2 * (yz - xw)
	m.M[8] = 2 * (xz - yw)
	m.M[9] = 2 * (yz + xw)
	m.M[10] = 1 - 2*(xx+yy)
	return m
}

// Polynomial represents a polynomial
type Polynomial struct {
	Coefficients []float64 // From highest degree to constant
}

// NewPolynomial creates a polynomial from coefficients
func NewPolynomial(coeffs []float64) *Polynomial {
	return &Polynomial{Coefficients: coeffs}
}

// Evaluate evaluates the polynomial at x
func (p *Polynomial) Evaluate(x float64) float64 {
	result := 0.0
	for _, coeff := range p.Coefficients {
		result = result*x + coeff
	}
	return result
}

// Derivative computes the derivative
func (p *Polynomial) Derivative() *Polynomial {
	if len(p.Coefficients) <= 1 {
		return NewPolynomial([]float64{0})
	}
	derivCoeffs := make([]float64, len(p.Coefficients)-1)
	for i, coeff := range p.Coefficients[:len(p.Coefficients)-1] {
		derivCoeffs[i] = coeff * float64(len(p.Coefficients)-1-i)
	}
	// Reverse to match Go slice order
	for i, j := 0, len(derivCoeffs)-1; i < j; i, j = i+1, j-1 {
		derivCoeffs[i], derivCoeffs[j] = derivCoeffs[j], derivCoeffs[i]
	}
	return NewPolynomial(derivCoeffs)
}
