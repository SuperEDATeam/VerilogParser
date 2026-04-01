autoidx 7
attribute \cells_not_processed 1
module \carry
  wire $1_Y
  wire $2_Y
  wire $3_Y
  wire $4_Y
  wire $5_Y
  wire $6_Y
  wire input 1 \a
  wire input 2 \b
  wire input 3 \c
  wire output 4 \a_equal_b
  wire output 5 \a_shift_b
  wire output 6 \cout
  wire \x
  cell $and $and$1
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \B_SIGNED 0
    parameter \B_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \a
    connect \B \b
    connect \Y $1_Y
  end
  cell $or $or$2
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \B_SIGNED 0
    parameter \B_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \a
    connect \B \b
    connect \Y $2_Y
  end
  cell $and $and$3
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \B_SIGNED 0
    parameter \B_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \b
    connect \B \c
    connect \Y $3_Y
  end
  cell $and $and$4
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \B_SIGNED 0
    parameter \B_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \a
    connect \B $3_Y
    connect \Y $4_Y
  end
  cell $or $or$5
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \B_SIGNED 0
    parameter \B_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \c
    connect \B \b
    connect \Y $5_Y
  end
  cell $or $or$6
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \B_SIGNED 0
    parameter \B_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \x
    connect \B $5_Y
    connect \Y $6_Y
  end
  connect \a_shift_b $1_Y
  connect \a_equal_b $2_Y
  connect \x $4_Y
  connect \cout $6_Y
end
